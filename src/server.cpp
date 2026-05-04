#include "server.hpp"

#include <exception>
#include <filesystem>
#include <fstream>
#include <glaze/core/opts.hpp>
#include <glaze/core/reflect.hpp>
#include <glaze/toml.hpp>
#include <glaze/toml/read.hpp>
#include <iterator>
#include <memory>
#include <print>
#include <fstream>

#include "asio/awaitable.hpp"
#include "asio/ip/address_v4.hpp"
#include "asio/ip/tcp.hpp"
#include "asio/socket_base.hpp"
#include "exceptions/bad_request.hpp"
#include "request.hpp"
#include "tcp_connection.hpp"

Server::Server(asio::io_context &io_context, std::string_view configPath) : acceptor(io_context) {
	this->context = &io_context;

	if(!std::filesystem::exists(configPath)){
		std::println("File {} does not exist", configPath);
		throw std::exception();
	}
	std::ifstream file{std::filesystem::path{configPath}};
	std::string fileData {std::istreambuf_iterator<char>{file}, std::istreambuf_iterator<char>{}};

	if(auto err = glz::read_toml<Config>(config, fileData); err){
		std::println("Error reading toml: {}", glz::format_error(err, fileData));
		throw std::exception();
	}
}

void Server::Start() {
	asio::ip::tcp::endpoint endpoint{asio::ip::make_address_v4(config.ip), config.port};
	acceptor.open(endpoint.protocol());
	acceptor.set_option(asio::ip::tcp::acceptor::reuse_address(true));
	acceptor.bind(endpoint);
	acceptor.listen(asio::socket_base::max_listen_connections);

	DoAccept();
	std::println("Started server on port {}", config.port);
}
void Server::Stop() {
	std::println("Stopping server");
	isClosing.store(true);
	acceptor.cancel();
	for (auto &connection : connections) {
		connection.second->close();
	}
	connections.clear();
	isClosing.store(false);
}

void Server::DoAccept() {
	acceptor.async_accept([this](const auto &err, auto socket) {
		if (err) {
			return;
		}

		std::shared_ptr<TcpConnection> connection{
			TcpConnection::create(std::move(socket), *this, connections.size())};
		connection->startReading();
		connections.insert({connections.size(), std::move(connection)});

		if (!isClosing.load()) {
			DoAccept();
		}
	});
}

void Server::Send(int connectionId, const char *data, std::size_t size) {
	auto it =
		std::find_if(connections.begin(), connections.end(),
					 [connectionId](const std::pair<int, std::shared_ptr<TcpConnection>> &conn) {
						 return conn.first == connectionId;
					 });

	if (it != connections.end()) {
		it->second->send(data, size);
	}
}
void Server::Disconnect(int connectionId) {
	auto it =
		std::find_if(connections.begin(), connections.end(),
					 [connectionId](const std::pair<int, std::shared_ptr<TcpConnection>> &conn) {
						 return conn.first == connectionId;
					 });

	if (it == connections.end()) {
		return;
	}

	connections.erase(it);
}

asio::awaitable<void> Server::OnReceived(int connectionId, const char *data, const std::size_t size) {
	HttpRequest request{std::string(data, data + size)};
	std::string exceptionMsg;

	if (filter.Filter(request)) {
		auto handler = controllers.GetPathHandler(request.header.method, request.header.path);

		try {
			if (!handler) {
				throw BadRequestException("No access");
			}

			auto value = co_await (*handler)(request);
			HttpHeader headers;
			headers.statusCode = "200 OK";

			HttpResponse response;
			response.header = headers;
			response.body = value;

			std::string stringified = response.Stringify();
			Send(connectionId, stringified.c_str(), stringified.size());
		} catch (std::exception &e) {
			exceptionMsg = e.what();
			std::println("Error serving {}: {}", request.header.path, e.what());
		}
	}

	auto response = HttpResponse::BadRequest(exceptionMsg);
	response.body.type = HttpResponse::BodyInfo::Type::PlainText;
	auto stringified = response.Stringify();

	Send(connectionId, stringified.c_str(), stringified.size());
	Disconnect(connectionId);
}
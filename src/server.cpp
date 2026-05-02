#include "server.hpp"

#include <exception>
#include <memory>
#include <print>

#include "asio/ip/address_v4.hpp"
#include "asio/ip/tcp.hpp"
#include "asio/socket_base.hpp"
#include "request.hpp"
#include "tcp_connection.hpp"

void Server::Start() {
	asio::ip::tcp::endpoint endpoint{asio::ip::make_address_v4(ip), port};
	acceptor.open(endpoint.protocol());
	acceptor.set_option(asio::ip::tcp::acceptor::reuse_address(true));
	acceptor.bind(endpoint);
	acceptor.listen(asio::socket_base::max_listen_connections);

	DoAccept();
	std::println("Started server");
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

		std::shared_ptr<TcpConnection> connection{TcpConnection::create(
			std::move(socket), *this, connections.size())};
		connection->startReading();
		connections.insert({connections.size(), std::move(connection)});

		if (!isClosing.load()) {
			DoAccept();
		}
	});
}

void Server::Send(int connectionId, const char *data, std::size_t size) {
	auto it = std::find_if(
		connections.begin(), connections.end(),
		[connectionId](
			const std::pair<int, std::shared_ptr<TcpConnection>> &conn) {
			return conn.first == connectionId;
		});

	if (it != connections.end()) {
		it->second->send(data, size);
	}
}
void Server::Disconnect(int connectionId) {
	auto it = std::find_if(
		connections.begin(), connections.end(),
		[connectionId](
			const std::pair<int, std::shared_ptr<TcpConnection>> &conn) {
			return conn.first == connectionId;
		});

	if (it == connections.end()) {
		return;
	}
	
	connections.erase(it);
}

void Server::OnReceived(int connectionId, const char *data,
						const std::size_t size) {
	HttpRequest request{std::string(data, data + size)};

	if (filter.Filter(request)) {
		auto handler = controllers.GetPathHandler(request.header.method, request.header.path);
		if (!handler) {
			std::println("Denied access");
			return;
		}

		try {
			auto value = (*handler)(request);
			HttpHeader headers;
			headers.statusCode = "200 OK";

			HttpRequest request;
			request.header = headers;
			if (!value.empty()) {
				request.body = value;
			}

			std::string stringified = request.Stringify();
			Send(connectionId, stringified.c_str(), stringified.size());
		} catch (std::exception &e) {
		}
	}

	std::string stringified = HttpRequest::BadRequest().Stringify();
	Send(connectionId, stringified.c_str(), stringified.size());
	Disconnect(connectionId);
}
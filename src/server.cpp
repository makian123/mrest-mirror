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

#include "asio/awaitable.hpp"
#include "asio/ip/address_v4.hpp"
#include "asio/ip/tcp.hpp"
#include "controller_manager.hpp"
#include "exceptions/bad_request.hpp"
#include "request.hpp"
#include "tcp_connection.hpp"

namespace mrest {
Server::Server(asio::io_context &io_context, std::string_view configPath) : acceptor(io_context), controllers(configPath) {
	this->context = &io_context;

	if (!std::filesystem::exists(configPath)) {
		std::println("File {} does not exist", configPath);
		throw std::exception();
	}
	std::ifstream file{std::filesystem::path{configPath}};
	std::string fileData{std::istreambuf_iterator<char>{file}, std::istreambuf_iterator<char>{}};

	struct ServerConfWrapper {
		Server::Config server;
	};
	ServerConfWrapper wrapper;

	if (auto err = glz::read<glz::opts{.format = glz::TOML,
		.error_on_unknown_keys = false}>(wrapper, fileData); err) {
		std::println("Error reading toml: {}", glz::format_error(err, fileData));
		throw std::exception();
	}

	config = wrapper.server;
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
		const auto id = connections.size();
		connection->startReading();
		connections.insert({id, std::move(connection)});

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

std::optional<HttpSession *> Server::GetSessionByUUID(std::string_view sessionId) {
	auto it = std::find_if(sessions.begin(), sessions.end(),
						   [sessionId](const std::pair<int, HttpSession> &session) {
							   return session.second.GetId() == sessionId;
						   });

	return it != sessions.end() ? std::optional(&it->second) : std::nullopt;
}

asio::awaitable<void> Server::OnReceived(int connectionId, const char *data,
										 const std::size_t size) {
	std::string exceptionMsg;
	HttpSession *connSession{nullptr};
	if (sessions.contains(connectionId)) {
		connSession = &sessions.at(connectionId);
	}
	HttpRequest request{*connSession, std::string(data, data + size)};

	// Check if session cookie exists
	bool appendSessionCookie{true};
	if (std::optional<const Cookie *> cookie =
			request.header.cookies.GetCookie(config.sessionCookieName)) {
		if (!connSession) {
			const Cookie *cookieVal = *cookie;
			auto gottenSession = GetSessionByUUID(cookieVal->value);
			if (gottenSession) {
				request.session = *gottenSession;
			}
		} else {
			appendSessionCookie = false;
		}
	}
	if(!request.session) {
		sessions.emplace(connectionId, HttpSession(3600));
		request.session = &sessions.at(connectionId);
	}

	if (!request.session) {
		exceptionMsg = "Session does not exist";
	}
	if(request.header.method == "OPTIONS"){
		HttpResponse response;
		response.header.statusCode = "200 OK";
		response.session = request.session;
		cors.AppendToResponse(response);

		std::string stringified = response.Stringify();
		request.body.type = HttpRequest::BodyInfo::Type::PlainText;

		Send(connectionId, stringified.c_str(), stringified.size());

		co_return;
	}
	// Serve request
	if (request.session && filter.Filter(request)) {
		auto handler = controllers.GetPathHandler(request.header.method, request.header.path);

		try {
			if (!handler) {
				throw exception::BadRequestException("No access");
			}

			auto value = co_await (*handler)(request);
			HttpHeader headers;
			headers.statusCode = "200 OK";

			HttpResponse response;
			response.header = headers;
			response.body = value;

			if (appendSessionCookie) {
				response.header.cookies.AddCookie(Cookie{std::string{config.sessionCookieName},
														 std::string{request.session->GetId()}});
			}
			cors.AppendToResponse(response);

			std::string stringified = response.Stringify();
			Send(connectionId, stringified.c_str(), stringified.size());

			co_return;
		} catch (std::exception &e) {
			exceptionMsg = e.what();
			std::println("Error serving {}: {}", request.header.path, e.what());
		}
	}

	auto response = HttpResponse::BadRequest(exceptionMsg);
	response.body.type = HttpResponse::BodyInfo::Type::PlainText;
	if (appendSessionCookie) {
		response.header.cookies.AddCookie(
			Cookie{std::string{config.sessionCookieName}, std::string{request.session->GetId()}});
	}

	auto stringified = response.Stringify();

	Send(connectionId, stringified.c_str(), stringified.size());
	Disconnect(connectionId);
	co_return;
}
}  // namespace mrest
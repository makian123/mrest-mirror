#pragma once

#include <cstdint>
#include <unordered_map>

#include "asio/awaitable.hpp"
#include "asio/io_context.hpp"
#include "asio/ip/tcp.hpp"
#include "controller_manager.hpp"
#include "cors.hpp"
#include "observer.hpp"
#include "request.hpp"
#include "request_interceptor.hpp"
#include "tcp_connection.hpp"

namespace mrest {
class Server : public Observer {
   public:
	Cors cors;

	struct Config {
		constexpr static std::uint64_t OneHourDuration = 60 * 60;  // 60 secs * 60 min
		std::string ip;
		std::uint16_t port;
		std::uint64_t sessionDuration{OneHourDuration};
		std::string sessionCookieName = "sessionID";
	};

	ControllerManager controllers;

	Server(asio::io_context &io_context, std::string_view configPath);
	~Server() { Stop(); }

	void AddFilter(FilterChain::FilterMethod method) { filter.AddBack(method); }
	void AddPreprocessor(Preprocessors::PreHandler handler) { preprocessors.AddBack(handler); }

	void Start();
	void Stop();

	void Send(int connectionId, const HttpResponse &response);
	void Send(int connectionId, const char *data, std::size_t size);
	void Disconnect(int connectionId);

	asio::awaitable<void> OnReceived(int connectionId, const std::vector<char> &data) override;

   private:
	FilterChain filter;
	Preprocessors preprocessors;
	asio::ip::tcp::acceptor acceptor;

	std::unordered_map<int, std::shared_ptr<TcpConnection>> connections;
	std::unordered_map<int, HttpSession> sessions;

	std::atomic_bool isClosing{false};

	Config config;

	void DoAccept();

	std::optional<HttpSession *> GetSessionByUUID(std::string_view sessionId);
};
}  // namespace mrest
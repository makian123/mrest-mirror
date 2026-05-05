#pragma once

#include <cstdint>
#include <unordered_map>

#include "asio/awaitable.hpp"
#include "asio/io_context.hpp"
#include "asio/ip/tcp.hpp"
#include "controller_manager.hpp"
#include "observer.hpp"
#include "request_interceptor.hpp"
#include "tcp_connection.hpp"

namespace mrest {
class Server : public Observer {
   public:
	struct Config {
		std::string ip;
		std::uint16_t port;
	};

	ControllerManager controllers;

	Server(asio::io_context &io_context, std::string_view configPath);
	~Server() { Stop(); }

	void AddFilter(FilterChain::FilterMethod method) { filter.AddBack(method); }

	void Start();
	void Stop();

	void Send(int connectionId, const char *data, std::size_t size);
	void Disconnect(int connectionId);

	asio::awaitable<void> OnReceived(int connectionId, const char *data,
									 const std::size_t size) override;

   private:
	FilterChain filter;
	asio::ip::tcp::acceptor acceptor;

	std::unordered_map<int, std::shared_ptr<TcpConnection>> connections;
	std::atomic_bool isClosing{false};

	Config config;

	void DoAccept();
};
}  // namespace mrest
#pragma once

#include <cstdint>
#include <unordered_map>

#include "asio/io_context.hpp"
#include "asio/ip/tcp.hpp"
#include "controller_manager.hpp"
#include "observer.hpp"
#include "request_interceptor.hpp"
#include "tcp_connection.hpp"

class Server: public Observer {
	FilterChain filter;
	std::string ip;
	std::uint16_t port;
	asio::ip::tcp::acceptor acceptor;

	std::unordered_map<int, std::shared_ptr<TcpConnection>> connections;
	std::atomic_bool isClosing{false};

	void DoAccept();
   public:
	ControllerManager controllers;

	Server(asio::io_context &io_context, std::string_view ip, std::uint16_t port)
		: acceptor(io_context), ip{std::string{ip}}, port{port} {}
	~Server() { Stop(); }

	void AddFilter(FilterChain::FilterMethod method) { filter.AddBack(method); }

	void Start();
	void Stop();

	void Send(int connectionId, const char *data, std::size_t size);
	void Disconnect(int connectionId);

	void OnReceived(int connectionId, const char *data, const std::size_t size) override;
};
#pragma once

#include <unistd.h>

#include <memory>
#include <mutex>

#include "asio/awaitable.hpp"
#include "asio/ip/tcp.hpp"
#include "asio/streambuf.hpp"
#include "observer.hpp"

namespace mrest {
class TcpConnection : public std::enable_shared_from_this<TcpConnection> {
	asio::ip::tcp::socket socket;
	asio::streambuf readBuf{};
	asio::streambuf writeBuf{};
	std::vector<char> buffer;
	bool keepAppending;
	int id;
	std::mutex writeMut{};
	bool isReading{false}, isWriting{false};
	Observer &observer;

	TcpConnection(asio::ip::tcp::socket &&socket, Observer &observer, int id);

	asio::awaitable<void> doRead();
	void doWrite();

   public:
	~TcpConnection() { close(); }
	static std::shared_ptr<TcpConnection> create(asio::ip::tcp::socket &&socket, Observer &observer,
												 int id = 0);

	void startReading();
	void send(const char *data, size_t size);
	void close();

	void SetAppending(bool keepAppending) { this->keepAppending = keepAppending; }
};
}  // namespace mrest
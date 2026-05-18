#include "tcp_connection.hpp"

#include <exception>
#include <iostream>
#include <mutex>
#include <ostream>
#include <print>

#include "asio/awaitable.hpp"
#include "asio/buffer.hpp"
#include "asio/buffers_iterator.hpp"
#include "asio/co_spawn.hpp"
#include "asio/detached.hpp"
#include "asio/io_context.hpp"
#include "asio/use_awaitable.hpp"
#include "observer.hpp"

namespace mrest {
TcpConnection::TcpConnection(asio::ip::tcp::socket &&sock, Observer &observer, int id)
	: socket{std::move(sock)}, observer{observer}, id{id} {}

std::shared_ptr<TcpConnection> TcpConnection::create(asio::ip::tcp::socket &&socket,
													 Observer &observer, int id) {
	return std::shared_ptr<TcpConnection>(new TcpConnection(std::move(socket), observer, id));
}

void TcpConnection::startReading() {
	if (!isReading) {
		co_spawn(*observer.context, doRead(), asio::detached);
	}
}
void TcpConnection::send(const char *data, size_t size) {
	std::lock_guard guard{writeMut};

	std::ostream outBuff(&writeBuf);
	outBuff.write(data, size);

	if (!isWriting) {
		doWrite();
	}
}
void TcpConnection::close() {
	try {
		socket.cancel();
		socket.close();
	} catch (const std::exception &e) {
		return;
	}
}

asio::awaitable<void> TcpConnection::doRead() {
	while (true) {
		std::size_t bytesTransferred =
			co_await socket.async_read_some(readBuf.prepare(1024), asio::use_awaitable);
		readBuf.commit(bytesTransferred);

		auto buffBegin = asio::buffers_begin(readBuf.data());
		buffer.insert(buffer.end(), buffBegin, buffBegin + bytesTransferred);

		co_await observer.OnReceived(id, buffer);

		if (!keepAppending) {
			buffer.clear();
		}

		readBuf.consume(bytesTransferred);
	}
	co_return;
}
void TcpConnection::doWrite() {
	isWriting = true;

	auto self{shared_from_this()};
	socket.async_write_some(writeBuf.data(), [this, self](const auto &err, auto bytesSent) {
		if (err) {
			return close();
		}

		std::lock_guard guard{writeMut};
		writeBuf.consume(bytesSent);
		if (writeBuf.size() == 0) {
			isWriting = false;
			return;
		}

		doWrite();
	});
}
}  // namespace mrest
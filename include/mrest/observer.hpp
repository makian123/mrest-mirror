#pragma once

#include <cstddef>

#include "asio/awaitable.hpp"
#include "asio/io_context.hpp"

namespace mrest {
struct Observer {
	asio::io_context *context;

	virtual asio::awaitable<void> OnReceived(int connectionId, const char *data,
											 const std::size_t size) {
		co_return;
	}
	virtual asio::awaitable<void> OnConnectionClosed(int connectionId) { co_return; }
};
}  // namespace mrest
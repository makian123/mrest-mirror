#pragma once

#include <cstddef>

struct Observer {
    virtual void OnReceived(int connectionId, const char *data, const std::size_t size){}
    virtual void OnConnectionClosed(int connectionId){}
};
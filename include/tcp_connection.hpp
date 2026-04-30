#pragma once

#include <memory>
#include <mutex>
#include "asio/ip/tcp.hpp"
#include "asio/streambuf.hpp"
#include "observer.hpp"

class TcpConnection: public std::enable_shared_from_this<TcpConnection> {
    asio::ip::tcp::socket socket;
    asio::streambuf readBuf{};
    asio::streambuf writeBuf{};
    int id;
    std::mutex writeMut{};
    bool isReading{false}, isWriting{false};
    Observer &observer;

    TcpConnection(asio::ip::tcp::socket &&socket, Observer &observer, int id);

    void doRead();
    void doWrite();
    public:
    static std::shared_ptr<TcpConnection> create(asio::ip::tcp::socket &&socket, Observer &observer, int id = 0);

    void startReading();
    void send(const char *data, size_t size);
    void close();
};
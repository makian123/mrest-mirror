#include "tcp_connection.hpp"
#include <exception>
#include <iostream>
#include <mutex>
#include <ostream>
#include "observer.hpp"

TcpConnection::TcpConnection(asio::ip::tcp::socket &&sock, Observer &observer, int id)
	: socket{std::move(sock)}, observer{observer}, id{id} {}

std::shared_ptr<TcpConnection> TcpConnection::create(
	asio::ip::tcp::socket &&socket, Observer &observer, int id) {
	return std::shared_ptr<TcpConnection>(new TcpConnection(std::move(socket), observer, id));
}

void TcpConnection::startReading(){
    if(!isReading){
        doRead();
    }
}
void TcpConnection::send(const char *data, size_t size){
    std::lock_guard guard{writeMut};

    std::ostream outBuff(&writeBuf);
    outBuff.write(data, size);

    if(!isWriting){
        doWrite();
    }
}
void TcpConnection::close(){
    try {
        socket.cancel();
        socket.close();
    }
    catch(const std::exception &e){
        return;
    }
}

void TcpConnection::doRead(){
    isReading = true;

    auto buffers{readBuf.prepare(1024)};
    auto self{shared_from_this()};

    socket.async_read_some(buffers, [this, self](const auto &err, auto bytesTransferred){
        if(err){
            return close();
        }

        readBuf.commit(bytesTransferred);
        observer.OnReceived(id, static_cast<const char*>(readBuf.data().data()), bytesTransferred);
        readBuf.consume(bytesTransferred);
        doRead();
    });
}
void TcpConnection::doWrite(){
    isWriting = true;

    auto self{shared_from_this()};
    socket.async_write_some(writeBuf.data(), [this, self](const auto &err, auto bytesSent){
        if(err){
            return close();
        }

        std::lock_guard guard{writeMut};
        writeBuf.consume(bytesSent);
        if(writeBuf.size() == 0){
            isWriting = false;
            return;
        }

        doWrite();
    });
}
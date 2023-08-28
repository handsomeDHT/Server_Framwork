//
// Created by 帅帅的涛 on 2023/8/28.
//

#ifndef SERVER_FRAMWORK_SOCKET_STREAM_H
#define SERVER_FRAMWORK_SOCKET_STREAM_H

#include "stream.h"
#include "socket.h"

namespace dht{

class SocketStream : public Stream {
public:
    typedef std::shared_ptr<SocketStream> ptr;
    SocketStream(Socket::ptr sock, bool owner = true);
    ~SocketStream();

    virtual int read(void* buffer, size_t length) override;
    virtual int read(ByteArray::ptr ba, size_t length) override;

    virtual int write(const void* buffer, size_t length) override;
    virtual int write(ByteArray::ptr ba, size_t length) override;

    virtual void close() override;

    Socket::ptr getSocket() const { return m_socket; }

    bool isConnected() const;

    Address::ptr getRemoteAddress();
    Address::ptr getLocalAddress();
    std::string getRemoteAddressString();
    std::string getLocalAddressString();
protected:
    Socket::ptr m_socket;
    bool m_owner;
private:
};

}

#endif //SERVER_FRAMWORK_SOCKET_STREAM_H

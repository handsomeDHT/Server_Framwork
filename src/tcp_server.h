//
// Created by 帅帅的涛 on 2023/8/24.
//

#ifndef SERVER_FRAMWORK_TCP_SERVER_H
#define SERVER_FRAMWORK_TCP_SERVER_H

#include <memory>
#include <functional>
#include "iomanager.h"
#include "socket.h"
#include "address.h"
#include "noncopyable.h"

namespace dht{

class TcpServer : public std::enable_shared_from_this<TcpServer>
                    , Noncopyable {
public:
    typedef std::shared_ptr<TcpServer> ptr;
    TcpServer(dht::IOManager* worker = dht::IOManager::GetThis()
            ,dht::IOManager* accept_worker = dht::IOManager::GetThis());
    virtual ~TcpServer();

    virtual bool bind(dht::Address::ptr addr);
    virtual bool bind(const std::vector<Address::ptr>& addrs
                      , std::vector<Address::ptr>& fails);
    virtual bool start();
    virtual bool stop();

    uint64_t getReadTimeout() const {return m_readTimeout;}
    std::string getName() const {return m_name;}
    bool isStop() const { return m_isStop; }

    void setReadTimeout(uint64_t v) { m_readTimeout = v; }
    void setName(std::string v) { m_name = v; }

protected:
    virtual void handleClient(Socket::ptr client);
    virtual void startAccept(Socket::ptr sock);
private:
    std::vector<Socket::ptr> m_socks;
    IOManager* m_worker;
    IOManager* m_acceptWorker;
    uint64_t m_readTimeout;
    std::string m_name;
    bool m_isStop;
};

}


#endif //SERVER_FRAMWORK_TCP_SERVER_H

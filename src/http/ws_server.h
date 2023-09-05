//
// Created by 帅帅的涛 on 2023/9/5.
//

#ifndef SERVER_FRAMWORK_WD_SERVER_H
#define SERVER_FRAMWORK_WD_SERVER_H


#include "src/tcp_server.h"
#include "ws_session.h"
#include "ws_servlet.h"

namespace dht {
namespace http {

class WSServer : public TcpServer {
public:
    typedef std::shared_ptr<WSServer> ptr;

    WSServer(dht::IOManager* worker = dht::IOManager::GetThis()
            , dht::IOManager* io_worker = dht::IOManager::GetThis()
            , dht::IOManager* accept_worker = dht::IOManager::GetThis());

    WSServletDispatch::ptr getWSServletDispatch() const { return m_dispatch;}
    void setWSServletDispatch(WSServletDispatch::ptr v) { m_dispatch = v;}
protected:
    virtual void handleClient(Socket::ptr client) override;
protected:
    WSServletDispatch::ptr m_dispatch;
};

}
}


#endif //SERVER_FRAMWORK_WD_SERVER_H

//
// Created by 帅帅的涛 on 2023/8/29.
//

#ifndef SERVER_FRAMWORK_HTTP_SERVER_H
#define SERVER_FRAMWORK_HTTP_SERVER_H

#include "src/tcp_server.h"
#include "http_session.h"
#include "servlet.h"

namespace dht{
namespace http{

class HttpServer : public TcpServer {
public:
    typedef std::shared_ptr<HttpServer> ptr;
    /**
    * @brief 构造函数
    * @param[in] keepalive 是否长连接
    * @param[in] worker 工作调度器
    * @param[in] accept_worker 接收连接调度器
    */
    HttpServer(bool keepalive = false
            ,dht::IOManager* worker = dht::IOManager::GetThis()
            ,dht::IOManager* accept_worker = dht::IOManager::GetThis());

    ServletDispatch::ptr getServletDispatch() const { return m_dispatch;}
    void setServletDispatch(ServletDispatch::ptr v) { m_dispatch = v;}

    virtual void setName(const std::string& v) override;
protected:
    virtual void handleClient(Socket::ptr client) override;
private:
    /// 是否支持长连接
    bool m_isKeepalive;
    /// Servlet分发器
    ServletDispatch::ptr m_dispatch;
};

}
}


#endif //SERVER_FRAMWORK_HTTP_SERVER_H

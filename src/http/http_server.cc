//
// Created by 帅帅的涛 on 2023/8/29.
//


#include "http_server.h"
#include "src/log.h"

namespace dht {
namespace http {

static dht::Logger::ptr g_logger = DHT_LOG_NAME("system");

HttpServer::HttpServer(bool keepalive
        ,dht::IOManager* worker
        ,dht::IOManager* accept_worker)
        :TcpServer(worker, accept_worker)
        ,m_isKeepalive(keepalive) {
    m_dispatch.reset(new ServletDispatch);
}

void HttpServer::handleClient(Socket::ptr client) {
    HttpSession::ptr session(new HttpSession(client));
    do {
        auto req = session->recvRequest();
        if(!req) {
            DHT_LOG_WARN(g_logger) << "recv http request fail, errno="
                                     << errno << " errstr=" << strerror(errno)
                                     << " cliet:" << *client;
            break;
        }

        HttpResponse::ptr rsp(new HttpResponse(req->getVersion()
                ,req->isClose() || !m_isKeepalive));
        rsp->setHeader("Server", getName());
        m_dispatch->handle(req, rsp, session);
//        rsp->setBody("hello dht");
//
//        DHT_LOG_INFO(g_logger) << "requst:" << std::endl
//            << *req;
//        DHT_LOG_INFO(g_logger) << "response:" << std::endl
//            << *rsp;
        session->sendResponse(rsp);
    } while(m_isKeepalive);
    session->close();
}

}
}
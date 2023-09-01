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
    DHT_LOG_DEBUG(g_logger) << "handleClient " << *client;

    // 创建一个 HttpSession 对象来处理客户端连接
    HttpSession::ptr session(new HttpSession(client));
    do {
        // 接收客户端的 HTTP 请求
        auto req = session->recvRequest();

        // 如果接收失败或者请求为空，则记录警告并退出循环
        if(!req) {
            DHT_LOG_WARN(g_logger) << "recv http request fail, errno="
                                     << errno << " errstr=" << strerror(errno)
                                     << " cliet:" << *client << " keep_alive=" << m_isKeepalive;
            break;
        }

        // 创建一个 HttpResponse 对象，设置 HTTP 版本和是否关闭连接
        HttpResponse::ptr rsp(new HttpResponse(req->getVersion()
                ,req->isClose() || !m_isKeepalive));
        // 设置响应头中的 Server 字段
        rsp->setHeader("Server", getName());
        // 调用调度器（dispatcher）处理请求和生成响应
        m_dispatch->handle(req, rsp, session);
        // 发送生成的响应给客户端
        session->sendResponse(rsp);
        // 如果不保持连接（非 keep-alive）或请求要求关闭连接，退出循环
        if(!m_isKeepalive || req->isClose()) {
            break;
        }
    } while(m_isKeepalive);
    session->close();
}

}
}

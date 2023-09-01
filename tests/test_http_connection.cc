//
// Created by 帅帅的涛 on 2023/9/1.
//

#include <iostream>
#include "src/http/http_connection.h"
#include "src/log.h"
#include "src/iomanager.h"
#include <fstream>

static dht::Logger::ptr g_logger = DHT_LOG_ROOT();

void run() {
    dht::Address::ptr addr = dht::Address::LookupAnyIPAddress("www.dht.top:80");
    if(!addr) {
        DHT_LOG_INFO(g_logger) << "get addr error";
        return;
    }

    dht::Socket::ptr sock = dht::Socket::CreateTCP(addr);
    bool rt = sock->connect(addr);
    if(!rt) {
        DHT_LOG_INFO(g_logger) << "connect " << *addr << " failed";
        return;
    }

    dht::http::HttpConnection::ptr conn(new dht::http::HttpConnection(sock));
    dht::http::HttpRequest::ptr req(new dht::http::HttpRequest);
    req->setPath("/blog/");
    req->setHeader("host", "www.dht.top");
    DHT_LOG_INFO(g_logger) << "req:" << std::endl
                             << *req;

    conn->sendRequest(req);
    auto rsp = conn->recvResponse();

    if(!rsp) {
        DHT_LOG_INFO(g_logger) << "recv response error";
        return;
    }
    DHT_LOG_INFO(g_logger) << "rsp:" << std::endl
                             << *rsp;

    std::ofstream ofs("rsp.dat");
    ofs << *rsp;
}

int main(int argc, char** argv) {
    dht::IOManager iom(2);
    iom.schedule(run);
    return 0;
}

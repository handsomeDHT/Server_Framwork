//
// Created by 帅帅的涛 on 2023/8/29.
//

#include "src/http/http_server.h"
#include "src/log.h"

static dht::Logger::ptr g_logger = DHT_LOG_ROOT();

void run() {
    dht::http::HttpServer::ptr server(new dht::http::HttpServer);
    dht::Address::ptr addr = dht::Address::LookupAnyIPAddress("0.0.0.0:8020");
    while(!server->bind(addr)) {
        sleep(2);
    }
    auto sd = server->getServletDispatch();
    sd->addServlet("/dht/xx", [](dht::http::HttpRequest::ptr req
            ,dht::http::HttpResponse::ptr rsp
            ,dht::http::HttpSession::ptr session) {
        rsp->setBody(req->toString());
        return 0;
    });

    sd->addGlobServlet("/dht/*", [](dht::http::HttpRequest::ptr req
            ,dht::http::HttpResponse::ptr rsp
            ,dht::http::HttpSession::ptr session) {
        rsp->setBody("Glob:\r\n" + req->toString());
        return 0;
    });
    server->start();
}

int main(int argc, char** argv) {
    dht::IOManager iom(2);
    iom.schedule(run);
    return 0;
}
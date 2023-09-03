//
// Created by 帅帅的涛 on 2023/9/3.
//

#include "src/http/http_server.h"
#include "src/log.h"

dht::Logger::ptr g_logger = DHT_LOG_ROOT();
dht::IOManager::ptr worker;
void run() {
    g_logger->setLevel(dht::LogLevel::INFO);
    dht::Address::ptr addr = dht::Address::LookupAnyIPAddress("0.0.0.0:8020");
    if(!addr) {
        DHT_LOG_ERROR(g_logger) << "get address error";
        return;
    }

    dht::http::HttpServer::ptr http_server(new dht::http::HttpServer(true, worker.get()));
    //dht::http::HttpServer::ptr http_server(new dht::http::HttpServer(true));
    bool ssl = false;
    while(!http_server->bind(addr, ssl)) {
        DHT_LOG_ERROR(g_logger) << "bind " << *addr << " fail";
        sleep(1);
    }

    if(ssl) {
        //http_server->loadCertificates("/home/apps/soft/dht/keys/server.crt", "/home/apps/soft/dht/keys/server.key");
    }

    http_server->start();
}

int main(int argc, char** argv) {
    dht::IOManager iom(1);
    worker.reset(new dht::IOManager(4, false));
    iom.schedule(run);
    return 0;
}

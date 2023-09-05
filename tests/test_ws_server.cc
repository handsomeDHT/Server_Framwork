#include "src/http/ws_server.h"
#include "src/log.h"

static dht::Logger::ptr g_logger = DHT_LOG_ROOT();

void run() {
    dht::http::WSServer::ptr server(new dht::http::WSServer);
    dht::Address::ptr addr = dht::Address::LookupAnyIPAddress("0.0.0.0:8020");
    if(!addr) {
        DHT_LOG_ERROR(g_logger) << "get address error";
        return;
    }
    auto fun = [](dht::http::HttpRequest::ptr header
            ,dht::http::WSFrameMessage::ptr msg
            ,dht::http::WSSession::ptr session) {
        session->sendMessage(msg);
        return 0;
    };

    server->getWSServletDispatch()->addServlet("/dht", fun);
    while(!server->bind(addr)) {
        DHT_LOG_ERROR(g_logger) << "bind " << *addr << " fail";
        sleep(1);
    }
    server->start();
}

int main(int argc, char** argv) {
    dht::IOManager iom(2);
    iom.schedule(run);
    return 0;
}

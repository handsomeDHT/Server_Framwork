//
// Created by 帅帅的涛 on 2023/8/25.
//

#include "src/tcp_server.h"
#include "src/iomanager.h"
#include "src/log.h"

dht::Logger::ptr g_logger = DHT_LOG_ROOT();

void run(){
    auto addr = dht::Address::LookupAny("0.0.0.0:8033");
    //auto addr2 = dht::UnixAddress::ptr(new dht::UnixAddress("/tmp/unix_addr"));
    std::vector<dht::Address::ptr> addrs;
    std::vector<dht::Address::ptr> fails;
    addrs.push_back(addr);
    //addrs.push_back(addr2);

    dht::TcpServer::ptr tcp_server(new dht::TcpServer);
    while(!tcp_server->bind(addrs,fails)){
        sleep(2);
    }
    tcp_server->start();

}

int main(int argc, char** argv) {
    dht::IOManager iom(2);
    iom.schedule(run);
    return 0;
}
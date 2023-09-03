//
// Created by 帅帅的涛 on 2023/9/3.
//

#include "src/socket.h"
#include "src/iomanager.h"
#include "src/log.h"
#include <stdlib.h>

static dht::Logger::ptr g_logger = DHT_LOG_ROOT();

const char* ip = nullptr;
uint16_t port = 0;

void run() {
    dht::IPAddress::ptr addr = dht::Address::LookupAnyIPAddress(ip);
    if(!addr) {
        DHT_LOG_ERROR(g_logger) << "invalid ip: " << ip;
        return;
    }
    addr->setPort(port);

    dht::Socket::ptr sock = dht::Socket::CreateUDP(addr);

    dht::IOManager::GetThis()->schedule([addr, sock](){
        DHT_LOG_INFO(g_logger) << "begin recv";
        while(true) {
            char buff[1024];
            int len = sock->recvFrom(buff, 1024, addr);
            if(len > 0) {
                std::cout << std::endl << "recv: " << std::string(buff, len) << " from: " << *addr << std::endl;
            }
        }
    });
    sleep(1);
    while(true) {
        std::string line;
        std::cout << "input>";
        std::getline(std::cin, line);
        if(!line.empty()) {
            int len = sock->sendTo(line.c_str(), line.size(), addr);
            if(len < 0) {
                int err = sock->getError();
                DHT_LOG_ERROR(g_logger) << "send error err=" << err
                                          << " errstr=" << strerror(err) << " len=" << len
                                          << " addr=" << *addr
                                          << " sock=" << *sock;
            } else {
                DHT_LOG_INFO(g_logger) << "send " << line << " len:" << len;
            }
        }
    }
}

int main(int argc, char** argv) {
    if(argc < 3) {
        DHT_LOG_INFO(g_logger) << "use as[" << argv[0] << " ip port]";
        return 0;
    }
    ip = argv[1];
    port = atoi(argv[2]);
    dht::IOManager iom(2);
    iom.schedule(run);
    return 0;
}

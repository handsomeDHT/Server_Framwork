//
// Created by 帅帅的涛 on 2023/9/3.
//
#include "src/socket.h"
#include "src/log.h"
#include "src/iomanager.h"

static dht::Logger::ptr g_logger = DHT_LOG_ROOT();

void run() {
    dht::IPAddress::ptr addr = dht::Address::LookupAnyIPAddress("0.0.0.0:8050");
    dht::Socket::ptr sock = dht::Socket::CreateUDP(addr);
    if(sock->bind(addr)) {
        DHT_LOG_INFO(g_logger) << "udp bind : " << *addr;
    } else {
        DHT_LOG_ERROR(g_logger) << "udp bind : " << *addr << " fail";
        return;
    }
    while(true) {
        char buff[1024];
        dht::Address::ptr from(new dht::IPv4Address);
        int len = sock->recvFrom(buff, 1024, from);
        if(len > 0) {
            buff[len] = '\0';
            DHT_LOG_INFO(g_logger) << "recv: " << buff << " from: " << *from;
            len = sock->sendTo(buff, len, from);
            if(len < 0) {
                DHT_LOG_INFO(g_logger) << "send: " << buff << " to: " << *from
                                         << " error=" << len;
            }
        }
    }
}

int main(int argc, char** argv) {
    dht::IOManager iom(1);
    iom.schedule(run);
    return 0;
}
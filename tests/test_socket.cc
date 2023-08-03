//
// Created by 帅帅的涛 on 2023/8/3.
//

#include "src/socket.h"
#include "src/dht.h"

static dht::Logger::ptr g_logger = DHT_LOG_ROOT();

void test_socket (){
    dht::IPAddress::ptr addr = dht::Address::LookupAnyIPAddress("www.baidu.com");
    if(addr){
        DHT_LOG_INFO(g_logger) << "get address: " << addr-> toString();
    } else {
        DHT_LOG_ERROR(g_logger) << "get address fail";
        return;
    };

    dht::Socket::ptr sock = dht::Socket::CreateTCP(addr);
    addr->setPort(80);
    DHT_LOG_INFO(g_logger) << "addr=" << addr->toString();
    if(!sock->connect(addr)) {
        DHT_LOG_ERROR(g_logger) << "connect " << addr->toString() << " fail";
        return;
    } else {
        DHT_LOG_INFO(g_logger) << "connect " << addr->toString() << " connected";
    }

    const char buff[] = "GET / HTTP/1.0\r\n\r\n";
    int rt = sock->send(buff, sizeof(buff));
    if(rt <= 0) {
        DHT_LOG_INFO(g_logger) << "send fail rt=" << rt;
        return;
    }

    std::string buffs;
    buffs.resize(4096);
    rt = sock->recv(&buffs[0], buffs.size());

    if(rt <= 0) {
        DHT_LOG_INFO(g_logger) << "recv fail rt=" << rt;
        return;
    }

    buffs.resize(rt);
    DHT_LOG_INFO(g_logger) << buffs;

};

int main(int argc, char** argv){
    dht::IOManager iom;
    iom.schedule(&test_socket);
    return 0;
}
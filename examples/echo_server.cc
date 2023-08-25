//
// Created by 帅帅的涛 on 2023/8/25.
//

#include "src/tcp_server.h"
#include "src/log.h"
#include "src/iomanager.h"
#include "src/bytearray.h"
#include "src/address.h"

static dht::Logger::ptr g_logger = DHT_LOG_ROOT();

class EchoServer : public dht::TcpServer {
public:
    EchoServer(int type);
    void handleClient(dht::Socket::ptr client);
private:
    int m_type = 0;
};

EchoServer::EchoServer(int type)
    :m_type(type){

}

void EchoServer::handleClient(dht::Socket::ptr client) {
    DHT_LOG_INFO(g_logger) << "handleClient" << *client;
    dht::ByteArray::ptr ba(new dht::ByteArray);
    while(true){
        ba->clear();
        std::vector<iovec> iovs;
        ba->getWriteBuffers(iovs, 1024);

        int rt = client->recv(&iovs[0], iovs.size());
        if(rt == 0){
            DHT_LOG_INFO(g_logger) << "client close: " << *client;
            break;
        } else if(rt < 0) {
            DHT_LOG_INFO(g_logger) << "client error: " << rt
                    << " errno=" << errno << "errstr= " <<strerror(errno);
            break;
        }
        ba->setPosition(ba->getPosition() + rt);
        ba->setPosition(0);
        if(m_type == 1) {//text
            DHT_LOG_INFO(g_logger) << ba->toString();
        } else {
            DHT_LOG_INFO(g_logger) << ba->toHexString();
        }
    }
}

int type = 1;
void run(){
    EchoServer::ptr es(new EchoServer(type));
    auto addr = dht::Address::LookupAny("0.0.0.0:8020");
    while(!es->bind(addr)){
        sleep(2);
    }
    es->start();
}

int main(int argc, char** argv){
    dht::IOManager iom(2);
    if(argc < 2){
        DHT_LOG_INFO(g_logger) << "used as[" << argv[0] << "-t] or [" << argv[0] << "-b]";
        return 0;
    }

    if(!strcmp(argv[1], "-b")){
        type = 2;
    }
    iom.schedule(run);
    return 0;
}
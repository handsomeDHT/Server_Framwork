//
// Created by 帅帅的涛 on 2023/7/12.
//done
//

#include "src/dht.h"
#include "src/iomanager.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>

dht::Logger::ptr g_logger = DHT_LOG_ROOT();

int sock = 0;

void test_fiber(){
    DHT_LOG_INFO(g_logger) << "test_fiber";

    sock = socket(AF_INET, SOCK_STREAM, 0);
    fcntl(sock, F_SETFL, O_NONBLOCK);

    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(80);
    inet_pton(AF_INET, "180.101.50.242", &addr.sin_addr.s_addr);

    if(!connect(sock, (const sockaddr*)&addr, sizeof(addr))) {
    } else if(errno == EINPROGRESS) {
        DHT_LOG_INFO(g_logger) << "add event errno=" << errno << " " << strerror(errno);
        dht::IOManager::GetThis()->addEvent(sock, dht::IOManager::READ, [](){
            DHT_LOG_INFO(g_logger) << "read callback";
        });
        dht::IOManager::GetThis()->addEvent(sock, dht::IOManager::WRITE, [](){
            DHT_LOG_INFO(g_logger) << "write callback";
            //close(sock);
            dht::IOManager::GetThis()->cancelEvent(sock, dht::IOManager::READ);
            close(sock);
        });
    } else {
        DHT_LOG_INFO(g_logger) << "else " << errno << " " << strerror(errno);
    }
}

void test1(){
    std::cout << "EPOLLIN=" << EPOLLIN
              << " EPOLLOUT=" << EPOLLOUT << std::endl;
    dht::IOManager iom(2, false);
    iom.schedule(&test_fiber);
}

dht::Timer::ptr s_timer;
void test_timer(){
    dht::IOManager iom(2);
    s_timer = iom.addTimer(1000, [](){
        static int i = 0;
        DHT_LOG_INFO(g_logger) << "hello timer i=" << i;
        if(++i == 3) {
            //s_timer->reset(2000, true);
            s_timer->cancel();
        }
    }, true);
}

int main(int argc, char** argv){
    test1();
    //test_timer();
    return 0;
}
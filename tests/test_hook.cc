//
// Created by 帅帅的涛 on 2023/7/18.
//
#include "src/dht.h"
#include "src/iomanager.h"
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>

dht::Logger::ptr g_logger = DHT_LOG_ROOT();

void test_sleep(){
    dht::IOManager iom(1);

    iom.schedule([](){
        sleep(2);
        DHT_LOG_INFO(g_logger) << "sleep 2";
    });

    iom.schedule([](){
        sleep(3);
        DHT_LOG_INFO(g_logger) << "sleep 3";
    });
    DHT_LOG_INFO(g_logger) << "test sleep";
}

void test_sock() {
    /**
     * 创建一个套接字，使用AF_INET表示IPv4地址族，SOCK_STREAM表示TCP协议，0表示默认的协议
     * @param[SOCK_STREAM] TCP套接字
     * @param[SOCK_DGRAM]  UDP套接字
     */
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr;

    /**
     * 用于将一段内存区域的值设置为特定的字节值(初始化活清除一块内存中的数据)
     * void *memset(void *ptr, int value, size_t num);
     * @param[ptr] 只想要设置的内存起始地址的指针
     * @param[value] 腰设置的字节值
     * @param[num] 要设置的字节数
     */
    memset(&addr, 0, sizeof(addr));

    addr.sin_family = AF_INET;  //使用IPV4地址族
    addr.sin_port = htons(80);  //链接端口80 (HTTP)
    inet_pton(AF_INET, "180.101.50.242", &addr.sin_addr.s_addr);// 将IP地址转换为二进制格式

    DHT_LOG_INFO(g_logger) << "begin connect";
    // 尝试通过创建的套接字和服务器地址连接到服务器
    int rt = connect(sock, (const sockaddr*)&addr, sizeof(addr));
    DHT_LOG_INFO(g_logger) << "connect rt=" << rt << " errno=" << errno;

    if(rt) {
        return;
    }
    // 准备要发送到服务器的HTTP请求数据
    const char data[] = "GET / HTTP/1.0\r\n\r\n";
    // 通过连接的套接字将HTTP请求发送到服务器
    rt = send(sock, data, sizeof(data), 0);
    DHT_LOG_INFO(g_logger) << "send rt=" << rt << " errno=" << errno;

    if(rt <= 0) {
        return;
    }
    // 准备一个缓冲区来存储接收到的数据（最多4096字节）
    std::string buff;
    buff.resize(4096);
    // 通过连接的套接字接收服务器的响应数据，并存储在缓冲区中
    rt = recv(sock, &buff[0], buff.size(), 0);
    DHT_LOG_INFO(g_logger) << "recv rt=" << rt << " errno=" << errno;

    if(rt <= 0) {
        return;
    }

    buff.resize(rt);
    DHT_LOG_INFO(g_logger) << buff;

}

int main(int argc, char** argv){
    //test_sleep();
    dht::IOManager iom;
    iom.schedule(test_sock);
    return 0;
}

//
// Created by 帅帅的涛 on 2023/7/31.
//

#include "socket.h"
#include "iomanager.h"
#include "fd_manager.h"
#include "log.h"
#include "macro.h"
#include "hook.h"
#include <netinet/tcp.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <limits.h>

namespace dht {

static dht::Logger::ptr g_logger = DHT_LOG_NAME("system");

/**
 * 创建不同种类的套接字
 */
Socket::ptr Socket::CreateTCP(dht::Address::ptr address) {
    // 创建一个新的Socket::ptr智能指针，使用指定的地址族和套接字类型（TCP）。
    Socket::ptr sock(new Socket(address->getFamily(), TCP, 0));
    return sock;
}

Socket::ptr Socket::CreateUDP(dht::Address::ptr address) {
    Socket::ptr sock(new Socket(address->getFamily(), UDP, 0));
    sock->newSock(); // 创建底层套接字句柄。
    sock->m_isConnected = true; // 标记套接字为已连接状态。
    return sock;
}

Socket::ptr Socket::CreateTCPSocket() {
    Socket::ptr sock(new Socket(IPv4, TCP, 0));
    return sock;
}

Socket::ptr Socket::CreateUDPSocket() {
    Socket::ptr sock(new Socket(IPv4, UDP, 0));
    sock->newSock();
    sock->m_isConnected = true;
    return sock;
}

Socket::ptr Socket::CreateTCPSocket6() {
    Socket::ptr sock(new Socket(IPv6, TCP, 0));
    return sock;
}

Socket::ptr Socket::CreateUDPSocket6() {
    Socket::ptr sock(new Socket(IPv6, UDP, 0));
    sock->newSock();
    sock->m_isConnected = true;
    return sock;
}

Socket::ptr Socket::CreateUnixTCPSocket() {
    Socket::ptr sock(new Socket(UNIX, TCP, 0));
    return sock;
}

Socket::ptr Socket::CreateUnixUDPSocket() {
    Socket::ptr sock(new Socket(UNIX, UDP, 0));
    return sock;
}

Socket::Socket(int family, int type, int protocol)
        :m_sock(-1)
        ,m_family(family)
        ,m_type(type)
        ,m_protocol(protocol)
        ,m_isConnected(false) {
}

Socket::~Socket() {
    close();
}

int64_t Socket::getSendTimeout() {
    // 通过套接字文件描述符获取对应的文件描述符上下文。
    FdCtx::ptr ctx = FdMgr::GetInstance()->get(m_sock);
    if(ctx) {
        // 如果上下文存在，调用其成员函数获取发送超时时间，使用 SO_SNDTIMEO 选项。
        return ctx->getTimeout(SO_SNDTIMEO);//SendTimeOut
    }
    // 如果上下文不存在，返回 -1，表示获取失败或未设置发送超时。
    return -1;
}

void Socket::setSendTimeout(int64_t v) {
    struct timeval tv{int(v / 1000), int(v % 1000 * 1000)};
    setOption(SOL_SOCKET, SO_SNDTIMEO, tv);
}

int64_t Socket::getRecvTimeout() {
    FdCtx::ptr ctx = FdMgr::GetInstance()->get(m_sock);
    if(ctx) {
        return ctx->getTimeout(SO_RCVTIMEO);
    }
    return -1;
}

void Socket::setRecvTimeout(int64_t v) {
    struct timeval tv{int(v / 1000), int(v % 1000 * 1000)};
    setOption(SOL_SOCKET, SO_RCVTIMEO, tv);
}

bool Socket::getOption(int level, int option, void* result, socklen_t* len) {
    int rt = getsockopt(m_sock, level, option, result, (socklen_t*)len);
    if(rt) {
        DHT_LOG_DEBUG(g_logger) << "getOption sock=" << m_sock
                                  << " level=" << level << " option=" << option
                                  << " errno=" << errno << " errstr=" << strerror(errno);
        return false;
    }
    return true;
}

bool Socket::setOption(int level, int option, const void* result, socklen_t len) {
    if(setsockopt(m_sock, level, option, result, (socklen_t)len)) {
        DHT_LOG_DEBUG(g_logger) << "setOption sock=" << m_sock
                                  << " level=" << level << " option=" << option
                                  << " errno=" << errno << " errstr=" << strerror(errno);
        return false;
    }
    return true;
}

Socket::ptr Socket::accept() {
    Socket::ptr sock(new Socket(m_family, m_type, m_protocol));
    // 使用系统调用 ::accept() 接受连接请求，获取新的套接字文件描述符。
    //::accept是全局作用域下的accept函数
    int newsock = ::accept(m_sock, nullptr, nullptr);
    if(newsock == -1) {
        DHT_LOG_ERROR(g_logger) << "accept(" << m_sock << ") errno="
                                  << errno << " errstr=" << strerror(errno);
        return nullptr;
    }
    // 初始化新的套接字对象，将其与新的套接字文件描述符绑定。
    if(sock->init(newsock)) {
        return sock;
    }
    return nullptr;
}

bool Socket::init(int sock) {
    FdCtx::ptr ctx = FdMgr::GetInstance()->get(sock);
    if(ctx && ctx->isSocket() && !ctx->isClose()) {
        m_sock = sock;
        m_isConnected = true;
        initSock();
        getLocalAddress();
        getRemoteAddress();
        return true;
    }
    return false;
}

bool Socket::bind(const Address::ptr addr) {
    //m_localAddress = addr;
    if(!isValid()) {
        newSock();
        if(DHT_UNLIKELY(!isValid())) {
            return false;
        }
    }

    if(DHT_UNLIKELY(addr->getFamily() != m_family)) {
        DHT_LOG_ERROR(g_logger) << "bind sock.family("
                                  << m_family << ") addr.family(" << addr->getFamily()
                                  << ") not equal, addr=" << addr->toString();
        return false;
    }

    UnixAddress::ptr uaddr = std::dynamic_pointer_cast<UnixAddress>(addr);
    if(uaddr) {
        Socket::ptr sock = Socket::CreateUnixTCPSocket();
        if(sock->connect(uaddr)) {
            return false;
        } else {
            dht::FSUtil::Unlink(uaddr->getPath(), true);
        }
    }

    /**
     * int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
     * @param[sockfd]需要绑定地址的套接字文件描述符。
     * @param[addr]指向 struct sockaddr 结构的指针，表示要绑定的地址。
     * @param[addrlen]结构的大小（字节数）。
     * @return 成功返回0，不成功返回1
     */
    if(::bind(m_sock, addr->getAddr(), addr->getAddrLen())) {
        DHT_LOG_ERROR(g_logger) << "bind error errrno=" << errno
                                  << " errstr=" << strerror(errno);
        return false;
    }
    getLocalAddress();
    return true;
}

bool Socket::reconnect(uint64_t timeout_ms) {
    //远端地址为空，说明没有链接对象
    if(!m_remoteAddress) {
        DHT_LOG_ERROR(g_logger) << "reconnect m_remoteAddress is null";
        return false;
    }
    m_localAddress.reset();
    return connect(m_remoteAddress, timeout_ms);
}

bool Socket::connect(const Address::ptr addr, uint64_t timeout_ms) {
    m_remoteAddress = addr;
    // 如果当前套接字无效，尝试创建一个新套接字
    if(!isValid()) {
        newSock();
        if(DHT_UNLIKELY(!isValid())) {
            return false;
        }
    }
    // 检查要连接的地址的地址族是否与当前套接字的地址族相同
    if(DHT_UNLIKELY(addr->getFamily() != m_family)) {
        DHT_LOG_ERROR(g_logger) << "connect sock.family("
                                  << m_family << ") addr.family(" << addr->getFamily()
                                  << ") not equal, addr=" << addr->toString();
        return false;
    }

    // 根据是否设置了连接超时时间，选择不同的连接方式
    if(timeout_ms == (uint64_t)-1) {
        if(::connect(m_sock, addr->getAddr(), addr->getAddrLen())) {
            DHT_LOG_ERROR(g_logger) << "sock=" << m_sock << " connect(" << addr->toString()
                                      << ") error errno=" << errno << " errstr=" << strerror(errno);
            close();
            return false;
        }
    } else {
        if(::connect_with_timeout(m_sock, addr->getAddr(), addr->getAddrLen(), timeout_ms)) {
            DHT_LOG_ERROR(g_logger) << "sock=" << m_sock << " connect(" << addr->toString()
                                      << ") timeout=" << timeout_ms << " error errno="
                                      << errno << " errstr=" << strerror(errno);
            close();
            return false;
        }
    }
    m_isConnected = true;
    getRemoteAddress();
    getLocalAddress();
    return true;
}

bool Socket::listen(int backlog) {
    if(!isValid()) {
        DHT_LOG_ERROR(g_logger) << "listen error sock=-1";
        return false;
    }
    /**
     * listen函数用来在服务器套接字上启动监听模式
     * 一旦套接字处于监听模式，它就可以排队等待客户端连接，然后使用 accept 函数来接受这些连接。
     * int listen(int sockfd, int backlog);
     * 1. 将指定的套接字设置为监听模式。这意味着该套接字将开始接受传入的连接请求。
     * 2. 创建一个等待连接的队列，可以存储一定数量的连接请求。如果队列已满，新的连接请求将被拒绝，直到有空间可用。
     * 3. 一旦套接字处于监听模式，服务器可以使用 accept 函数从队列中取出连接请求并建立连接。
     */
    if(::listen(m_sock, backlog)) {
        DHT_LOG_ERROR(g_logger) << "listen error errno=" << errno
                                  << " errstr=" << strerror(errno);
        return false;
    }
    return true;
}

bool Socket::close() {
    if(!m_isConnected && m_sock == -1) {
        return true;
    }
    m_isConnected = false;
    if(m_sock != -1) {
        ::close(m_sock);
        m_sock = -1;
    }
    return false;
}

// 发送连续的数据块到已连接的套接字
int Socket::send(const void* buffer, size_t length, int flags) {
    if(isConnected()) {
        return ::send(m_sock, buffer, length, flags);
    }
    return -1;
}

// 发送分散的数据块到已连接的套接字
/**
 * iovec 是用于描述分散/聚集 I/O（Scatter/Gather I/O）操作的结构体，在许多操作系统中用于优化数据的传输。
 * 它允许你在单个 I/O 操作中同时传输多个不连续的数据块（缓冲区），从而避免了数据复制的开销。
 * 在网络编程中，使用 iovec 结构体可以有效地将多个数据块一次性传输到套接字或从套接字接收到多个数据块。
 * 这对于处理大量分散的数据非常有用，可以提高数据传输的效率。
 */
int Socket::send(const iovec* buffers, size_t length, int flags) {
    if(isConnected()) {
        msghdr msg;
        memset(&msg, 0, sizeof(msg));
        msg.msg_iov = (iovec*)buffers;
        msg.msg_iovlen = length;
        return ::sendmsg(m_sock, &msg, flags);
    }
    return -1;
}
/**
 * send 适用于已连接的套接字，不需要在每次发送时指定目标地址，适用于 TCP 协议。
 * sendto 适用于无连接的套接字，需要在每次发送时指定目标地址，适用于 UDP 协议。
 */
int Socket::sendTo(const void* buffer, size_t length, const Address::ptr to, int flags) {
    if(isConnected()) {
        return ::sendto(m_sock, buffer, length, flags, to->getAddr(), to->getAddrLen());
    }
    return -1;
}

int Socket::sendTo(const iovec* buffers, size_t length, const Address::ptr to, int flags) {
    if(isConnected()) {
        msghdr msg;
        memset(&msg, 0, sizeof(msg));
        msg.msg_iov = (iovec*)buffers;
        msg.msg_iovlen = length;
        msg.msg_name = to->getAddr();
        msg.msg_namelen = to->getAddrLen();
        return ::sendmsg(m_sock, &msg, flags);
    }
    return -1;
}
/**
 * ssize_t recv(int sockfd, void *buf, size_t len, int flags);
 * @param[fd] 套接字文件描述符。
 * @param[buf] 指向接收数据的缓冲区的指针。
 * @param[len] 要接收的数据的最大长度（字节数）。
 * @param[flags] 接收操作的标志位，通常为 0。
 *
 * @return 如果成功接收数据，返回接收到的字节数
 *         如果连接被关闭，recv返回0，表示已经关闭连接
 *         如果发生错误，返回-1，并使用全局变量errno来表示错误类型
 */
int Socket::recv(void* buffer, size_t length, int flags) {
    if(isConnected()) {
        return ::recv(m_sock, buffer, length, flags);
    }
    return -1;
}

/**
 * struct msghdr{
 *      void         *msg_name;       // 指向目标/源地址的指针
        socklen_t     msg_namelen;    // 地址结构的长度
        struct iovec *msg_iov;        // 指向数据块的数组
        size_t        msg_iovlen;     // 数据块的数量
        void         *msg_control;    // 指向控制数据的指针
        socklen_t     msg_controllen; // 控制数据的长度
        int           msg_flags;      // 消息标志
 * }
 */
int Socket::recv(iovec* buffers, size_t length, int flags) {
    if(isConnected()) {
        msghdr msg;
        //初始一块内存空间
        memset(&msg, 0, sizeof(msg));
        msg.msg_iov = (iovec*)buffers;
        msg.msg_iovlen = length;
        return ::recvmsg(m_sock, &msg, flags);
    }
    return -1;
}

int Socket::recvFrom(void* buffer, size_t length, Address::ptr from, int flags) {
    if(isConnected()) {
        socklen_t len = from->getAddrLen();
        return ::recvfrom(m_sock, buffer, length, flags, from->getAddr(), &len);
    }
    return -1;
}

int Socket::recvFrom(iovec* buffers, size_t length, Address::ptr from, int flags) {
    if(isConnected()) {
        msghdr msg;
        memset(&msg, 0, sizeof(msg));
        msg.msg_iov = (iovec*)buffers;
        msg.msg_iovlen = length;
        msg.msg_name = from->getAddr();
        msg.msg_namelen = from->getAddrLen();
        return ::recvmsg(m_sock, &msg, flags);
    }
    return -1;
}

Address::ptr Socket::getRemoteAddress() {
    if(m_remoteAddress) {
        return m_remoteAddress;
    }

    Address::ptr result;
    switch(m_family) {
        case AF_INET:
            result.reset(new IPv4Address());
            break;
        case AF_INET6:
            result.reset(new IPv6Address());
            break;
        case AF_UNIX:
            result.reset(new UnixAddress());
            break;
        default:
            result.reset(new UnknownAddress(m_family));
            break;
    }
    socklen_t addrlen = result->getAddrLen();
    /**
     * 使用 getpeername 函数来获取已连接套接字的对端地址。
     * 如果失败，返回一个表示未知地址的 UnknownAddress 对象
     * int getpeername(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
     * @param[sockfd]已连接套接字的文件描述符。
     * @param[addr] 指向一个用于存储对端地址的 sockaddr 结构体的指针。
     * @param[addrlen] 指向一个整数的指针，用于存储 addr 结构体的大小。调用之前需要将其设置为 addr 缓冲区的大小，函数调用后会将其更新为实际地址的大小。
     */
    if(getpeername(m_sock, result->getAddr(), &addrlen)) {
        return Address::ptr(new UnknownAddress(m_family));
    }
    if(m_family == AF_UNIX) {
        UnixAddress::ptr addr = std::dynamic_pointer_cast<UnixAddress>(result);
        addr->setAddrLen(addrlen);
    }
    m_remoteAddress = result;
    return m_remoteAddress;
}

Address::ptr Socket::getLocalAddress() {
    if(m_localAddress) {
        return m_localAddress;
    }

    Address::ptr result;
    switch(m_family) {
        case AF_INET:
            result.reset(new IPv4Address());
            break;
        case AF_INET6:
            result.reset(new IPv6Address());
            break;
        case AF_UNIX:
            result.reset(new UnixAddress());
            break;
        default:
            result.reset(new UnknownAddress(m_family));
            break;
    }
    socklen_t addrlen = result->getAddrLen();
    if(getsockname(m_sock, result->getAddr(), &addrlen)) {
        DHT_LOG_ERROR(g_logger) << "getsockname error sock=" << m_sock
                                  << " errno=" << errno << " errstr=" << strerror(errno);
        return Address::ptr(new UnknownAddress(m_family));
    }
    if(m_family == AF_UNIX) {
        UnixAddress::ptr addr = std::dynamic_pointer_cast<UnixAddress>(result);
        addr->setAddrLen(addrlen);
    }
    m_localAddress = result;
    return m_localAddress;
}

bool Socket::isValid() const {
    return m_sock != -1;
}

int Socket::getError() {
    int error = 0;
    socklen_t len = sizeof(error);
    if(!getOption(SOL_SOCKET, SO_ERROR, &error, &len)) {
        error = errno;
    }
    return error;
}
//将给定的信息输出成流
std::ostream& Socket::dump(std::ostream& os) const {
    os << "[Socket sock=" << m_sock
       << " is_connected=" << m_isConnected
       << " family=" << m_family
       << " type=" << m_type
       << " protocol=" << m_protocol;
    if(m_localAddress) {
        os << " local_address=" << m_localAddress->toString();
    }
    if(m_remoteAddress) {
        os << " remote_address=" << m_remoteAddress->toString();
    }
    os << "]";
    return os;
}

std::string Socket::toString() const {
    std::stringstream ss;
    dump(ss);
    return ss.str();
}

bool Socket::cancelRead() {
    return IOManager::GetThis()->cancelEvent(m_sock, dht::IOManager::READ);
}

bool Socket::cancelWrite() {
    return IOManager::GetThis()->cancelEvent(m_sock, dht::IOManager::WRITE);
}

bool Socket::cancelAccept() {
    return IOManager::GetThis()->cancelEvent(m_sock, dht::IOManager::READ);
}

bool Socket::cancelAll() {
    return IOManager::GetThis()->cancelAll(m_sock);
}

void Socket::initSock() {
    int val = 1;
    setOption(SOL_SOCKET, SO_REUSEADDR, val);
    if(m_type == SOCK_STREAM) {
        setOption(IPPROTO_TCP, TCP_NODELAY, val);
    }
}

void Socket::newSock() {
    m_sock = socket(m_family, m_type, m_protocol);
    if(DHT_LIKELY(m_sock != -1)) {
        initSock();
    } else {
        DHT_LOG_ERROR(g_logger) << "socket(" << m_family
                                  << ", " << m_type << ", " << m_protocol << ") errno="
                                  << errno << " errstr=" << strerror(errno);
    }
}

namespace {

struct _SSLInit {
    _SSLInit() {
        SSL_library_init();
        SSL_load_error_strings();
        OpenSSL_add_all_algorithms();
    }
};

static _SSLInit s_init;

}

SSLSocket::SSLSocket(int family, int type, int protocol)
        :Socket(family, type, protocol) {
}

Socket::ptr SSLSocket::accept() {
    SSLSocket::ptr sock(new SSLSocket(m_family, m_type, m_protocol));
    int newsock = ::accept(m_sock, nullptr, nullptr);
    if(newsock == -1) {
        DHT_LOG_ERROR(g_logger) << "accept(" << m_sock << ") errno="
                                  << errno << " errstr=" << strerror(errno);
        return nullptr;
    }
    sock->m_ctx = m_ctx;
    if(sock->init(newsock)) {
        return sock;
    }
    return nullptr;
}

bool SSLSocket::bind(const Address::ptr addr) {
    return Socket::bind(addr);
}

bool SSLSocket::connect(const Address::ptr addr, uint64_t timeout_ms) {
    bool v = Socket::connect(addr, timeout_ms);
    if(v) {
        m_ctx.reset(SSL_CTX_new(SSLv23_client_method()), SSL_CTX_free);
        m_ssl.reset(SSL_new(m_ctx.get()),  SSL_free);
        SSL_set_fd(m_ssl.get(), m_sock);
        v = (SSL_connect(m_ssl.get()) == 1);
    }
    return v;
}

bool SSLSocket::listen(int backlog) {
    return Socket::listen(backlog);
}

bool SSLSocket::close() {
    return Socket::close();
}

int SSLSocket::send(const void* buffer, size_t length, int flags) {
    if(m_ssl) {
        return SSL_write(m_ssl.get(), buffer, length);
    }
    return -1;
}

int SSLSocket::send(const iovec* buffers, size_t length, int flags) {
    if(!m_ssl) {
        return -1;
    }
    int total = 0;
    for(size_t i = 0; i < length; ++i) {
        int tmp = SSL_write(m_ssl.get(), buffers[i].iov_base, buffers[i].iov_len);
        if(tmp <= 0) {
            return tmp;
        }
        total += tmp;
        if(tmp != (int)buffers[i].iov_len) {
            break;
        }
    }
    return total;
}

int SSLSocket::sendTo(const void* buffer, size_t length, const Address::ptr to, int flags) {
    DHT_ASSERT(false);
    return -1;
}

int SSLSocket::sendTo(const iovec* buffers, size_t length, const Address::ptr to, int flags) {
    DHT_ASSERT(false);
    return -1;
}

int SSLSocket::recv(void* buffer, size_t length, int flags) {
    if(m_ssl) {
        return SSL_read(m_ssl.get(), buffer, length);
    }
    return -1;
}

int SSLSocket::recv(iovec* buffers, size_t length, int flags) {
    if(!m_ssl) {
        return -1;
    }
    int total = 0;
    for(size_t i = 0; i < length; ++i) {
        int tmp = SSL_read(m_ssl.get(), buffers[i].iov_base, buffers[i].iov_len);
        if(tmp <= 0) {
            return tmp;
        }
        total += tmp;
        if(tmp != (int)buffers[i].iov_len) {
            break;
        }
    }
    return total;
}

int SSLSocket::recvFrom(void* buffer, size_t length, Address::ptr from, int flags) {
    DHT_ASSERT(false);
    return -1;
}

int SSLSocket::recvFrom(iovec* buffers, size_t length, Address::ptr from, int flags) {
    DHT_ASSERT(false);
    return -1;
}

bool SSLSocket::init(int sock) {
    bool v = Socket::init(sock);
    if(v) {
        m_ssl.reset(SSL_new(m_ctx.get()),  SSL_free);
        SSL_set_fd(m_ssl.get(), m_sock);
        v = (SSL_accept(m_ssl.get()) == 1);
    }
    return v;
}

bool SSLSocket::loadCertificates(const std::string& cert_file, const std::string& key_file) {
    m_ctx.reset(SSL_CTX_new(SSLv23_server_method()), SSL_CTX_free);
    if(SSL_CTX_use_certificate_chain_file(m_ctx.get(), cert_file.c_str()) != 1) {
        DHT_LOG_ERROR(g_logger) << "SSL_CTX_use_certificate_chain_file("
                                  << cert_file << ") error";
        return false;
    }
    if(SSL_CTX_use_PrivateKey_file(m_ctx.get(), key_file.c_str(), SSL_FILETYPE_PEM) != 1) {
        DHT_LOG_ERROR(g_logger) << "SSL_CTX_use_PrivateKey_file("
                                  << key_file << ") error";
        return false;
    }
    if(SSL_CTX_check_private_key(m_ctx.get()) != 1) {
        DHT_LOG_ERROR(g_logger) << "SSL_CTX_check_private_key cert_file="
                                  << cert_file << " key_file=" << key_file;
        return false;
    }
    return true;
}

SSLSocket::ptr SSLSocket::CreateTCP(dht::Address::ptr address) {
    SSLSocket::ptr sock(new SSLSocket(address->getFamily(), TCP, 0));
    return sock;
}

SSLSocket::ptr SSLSocket::CreateTCPSocket() {
    SSLSocket::ptr sock(new SSLSocket(IPv4, TCP, 0));
    return sock;
}

SSLSocket::ptr SSLSocket::CreateTCPSocket6() {
    SSLSocket::ptr sock(new SSLSocket(IPv6, TCP, 0));
    return sock;
}

std::ostream& SSLSocket::dump(std::ostream& os) const {
    os << "[SSLSocket sock=" << m_sock
       << " is_connected=" << m_isConnected
       << " family=" << m_family
       << " type=" << m_type
       << " protocol=" << m_protocol;
    if(m_localAddress) {
        os << " local_address=" << m_localAddress->toString();
    }
    if(m_remoteAddress) {
        os << " remote_address=" << m_remoteAddress->toString();
    }
    os << "]";
    return os;
}

std::ostream& operator<<(std::ostream& os, const Socket& sock) {
    return sock.dump(os);
}


}

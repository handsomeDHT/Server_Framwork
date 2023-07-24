//
// Created by 帅帅的涛 on 2023/7/18.
//

#include "hook.h"
#include "fiber.h"
#include "iomanager.h"
#include "fd_manager.h"
#include "dht.h"
#include <functional>
#include <dlfcn.h>

dht::Logger::ptr g_logger = DHT_LOG_NAME("system");
namespace dht {

static dht::ConfigVar<int>::ptr g_tcp_connect_timeout
            = dht::Config::Lookup("tcp.connect.timeout", 5000, "tcp connect timeout");
static thread_local bool t_hook_enable = false;

#define HOOK_FUN(XX) \
    XX(sleep) \
    XX(usleep) \
    XX(nanosleep) \
    XX(socket) \
    XX(connect) \
    XX(accept) \
    XX(read) \
    XX(readv) \
    XX(recv) \
    XX(recvfrom) \
    XX(recvmsg) \
    XX(write) \
    XX(writev) \
    XX(send) \
    XX(sendto) \
    XX(sendmsg) \
    XX(close) \
    XX(fcntl) \
    XX(ioctl) \
    XX(getsockopt) \
    XX(setsockopt)

void hook_init() {
    static bool is_inited = false;
    if (is_inited) {
        return;
    }
#define XX(name) name ## _f = (name ## _fun)dlsym(RTLD_NEXT, #name);
    HOOK_FUN(XX);
#undef XX
}

static uint64_t s_connect_timeout = -1;

//因为在执行main函数前，会先将所有全局变量的定义进行执行
//所以可以利用这个特性，定义一些在main函数之前就会执行的方法
struct _HookIniter {
    _HookIniter() {
        hook_init();
        s_connect_timeout = g_tcp_connect_timeout->getValue();

        g_tcp_connect_timeout->addListener([](const int& old_value, const int& new_value){
            DHT_LOG_INFO(g_logger) << "tcp connect timeout changed from "
                                     << old_value << " to " << new_value;
            s_connect_timeout = new_value;
        });
    }
};

static _HookIniter s_hook_initer;

bool is_hook_enable() {
    return t_hook_enable;
}

void set_hook_enable(bool flag) {
    t_hook_enable = flag;
}
}

struct timer_info{
    int cancelled = 0;
};

//该函数主要用于在进行 I/O 操作前，根据不同情况对套接字进行一些预处理，
//如判断套接字是否可用、是否需要设置超时等，并且提供了对非阻塞 I/O 和超时 I/O 的支持。
template<typename OriginFun, typename... Args>
static ssize_t do_io(int fd, OriginFun fun, const char* hook_fun_name
        , uint32_t event, int timeout_so, Args&&... args){
    if(!dht::t_hook_enable){
        return fun(fd, std::forward<Args>(args)...);
    }

    DHT_LOG_DEBUG(g_logger) << "do_io" << hook_fun_name << ">";

    // 获取与给定套接字 fd 相关联的 FdCtx（文件描述符上下文）
    dht::FdCtx::ptr ctx = dht::FdMgr::GetInstance()->get(fd);
    if(!ctx){
        return fun(fd, std::forward<Args>(args)...);
    }
    // 如果该套接字已经关闭，则返回 EBADF 错误
    if(ctx->isClose()){
        errno = EBADF;
        return -1;
    }
    // 如果不是套接字类型，或者用户设置了非阻塞模式，则直接调用原始函数 fun 并返回结果
    if(!ctx->isSocket() || ctx->getUserNonblock()) {
        return fun(fd, std::forward<Args>(args)...);
    }
    // 获取超时时间
    uint64_t to = ctx->getTimeout(timeout_so);
    // 创建一个 timer_info 结构的智能指针 tinfo
    std::shared_ptr<timer_info> tinfo(new timer_info);

retry:
    // 调用原始函数 fun 进行 I/O 操作
    ssize_t n = fun(fd, std::forward<Args>(args)...);
    // 处理因信号中断而返回 -1 的情况，继续调用 fun 进行重试
    while (n == -1 && errno == EINTR) {
        n = fun(fd, std::forward<Args>(args)...);
    }
    // 如果操作返回 -1，并且错误码为 EAGAIN（资源暂时不可用，通常表示非阻塞 I/O 中无数据可读/写）
    if(n == -1 && errno == EAGAIN) {
        // 获取 IOManager 的实例指针
        dht::IOManager* iom = dht::IOManager::GetThis();
        // 创建一个 Timer 实例，用于超时计时
        dht::Timer::ptr timer;
        // 创建一个指向 tinfo 的弱引用，用于后续回调中判断 tinfo 是否被取消
        std::weak_ptr<timer_info> winfo(tinfo);
        // 如果设置了超时时间，则创建定时器
        if(to != (uint64_t)-1){
            timer = iom->addConditionTimer(to, [winfo, fd, iom, event](){
                auto t = winfo.lock();
                if(!t || t->cancelled){
                    return;
                }
                t->cancelled = ETIMEDOUT;
                iom->cancelEvent(fd, (dht::IOManager::Event)(event));
            }, winfo);
        }
        //int c = 0;
        //uint64_t now = 0;
        // 将套接字添加到 IOManager 中，等待事件发生
        int rt = iom->addEvent(fd, (dht::IOManager::Event)(event));
        if(rt){
                DHT_LOG_ERROR(g_logger) << hook_fun_name << " addEvent("
                                        << fd << ", " << event << ")";
            if(timer) {
                timer->cancel();
            }
            return -1;
        }else {
            // 将当前协程切出，让出执行权给其他协程
            dht::Fiber::YieldToHold();
            if(timer) {
                timer->cancel();
            }
            // 如果 tinfo 被标记为取消，返回相应的错误码
            if(tinfo->cancelled) {
                errno = tinfo->cancelled;
                return -1;
            }
            // 回到 retry 标签处，继续进行重试操作
            goto retry;
        }
    }
    return n;
}

extern "C" {
#define XX(name) name ## _fun name ## _f = nullptr;
    HOOK_FUN(XX);
#undef XX
/**
* sleep
*/
unsigned int sleep (unsigned int seconds){
    if(!dht::t_hook_enable) {
        return sleep_f(seconds);
    }
    dht::Fiber::ptr fiber = dht::Fiber::GetThis();
    dht::IOManager* iom = dht::IOManager::GetThis();
    iom->addTimer(seconds * 1000, std::bind((void(dht::Scheduler::*)
                                                    (dht::Fiber::ptr, int thread))&dht::IOManager::schedule
            ,iom, fiber, -1));
    dht::Fiber::YieldToHold();
    return 0;
}

int usleep(useconds_t usec){
    if(!dht::t_hook_enable){
        return usleep_f(usec);
    }
    dht::Fiber::ptr fiber = dht::Fiber::GetThis();
    dht::IOManager* iom = dht::IOManager::GetThis();
    iom->addTimer(usec / 1000, std::bind((void(dht::Scheduler::*)
                                                 (dht::Fiber::ptr, int thread))&dht::IOManager::schedule
            ,iom, fiber, -1));
    dht::Fiber::YieldToHold();
    return 0;
}

int nanosleep(const struct timespec *req, struct timespec *rem){
    if(!dht::t_hook_enable){
        return nanosleep_f(req,rem);
    }
    int timeout_ms = req->tv_sec * 1000 + req->tv_nsec /1000 / 1000;
    dht::Fiber::ptr fiber = dht::Fiber::GetThis();
    dht::IOManager* iom = dht::IOManager::GetThis();
    iom->addTimer(timeout_ms, std::bind((void(dht::Scheduler::*)
                                                (dht::Fiber::ptr, int thread))&dht::IOManager::schedule
            ,iom, fiber, -1));
    dht::Fiber::YieldToHold();
    return 0;
}

/**
 * socket API
 */

int socket(int domain, int type, int protocol) {
    if(!dht::t_hook_enable) {
        return socket_f(domain, type, protocol);
    }
    int fd = socket_f(domain, type, protocol);
    if(fd == -1) {
        return fd;
    }
    dht::FdMgr::GetInstance()->get(fd, true);
    return fd;
}

int connect_with_timeout(int fd, const struct sockaddr* addr, socklen_t addrlen, uint64_t timeout_ms) {
    if(!dht::t_hook_enable) {
        return connect_f(fd, addr, addrlen);
    }
    // 获取给定套接字 fd 的文件描述符(句柄)
    dht::FdCtx::ptr ctx = dht::FdMgr::GetInstance()->get(fd);
    if(!ctx || ctx->isClose()) {
        errno = EBADF;
        return -1;
    }
    // 如果不是套接字类型，或者用户设置了非阻塞模式，则直接调用原始的 connect 函数
    if(!ctx->isSocket()) {
        return connect_f(fd, addr, addrlen);
    }
    // 尝试进行连接操作，如果连接成功返回 0，如果连接失败且错误码不是 EINPROGRESS，则返回相应的错误码
    if(ctx->getUserNonblock()) {
        return connect_f(fd, addr, addrlen);
    }

    int n = connect_f(fd, addr, addrlen);
    if(n == 0) {
        return 0;
    } else if(n != -1 || errno != EINPROGRESS) {
        return n;
    }

    dht::IOManager* iom = dht::IOManager::GetThis();
    dht::Timer::ptr timer;
    // 创建一个 timer_info 结构的智能指针 tinfo，并创建指向它的弱引用 winfo
    std::shared_ptr<timer_info> tinfo(new timer_info);
    std::weak_ptr<timer_info> winfo(tinfo);
    // 如果设置了超时时间，则创建定时器
    if(timeout_ms != (uint64_t)-1) {
        timer = iom->addConditionTimer(timeout_ms, [winfo, fd, iom]() {
            auto t = winfo.lock();
            if(!t || t->cancelled) {
                return;
            }
            t->cancelled = ETIMEDOUT;
            iom->cancelEvent(fd, dht::IOManager::WRITE);
        }, winfo);
    }
    // 将套接字添加到 IOManager 中，等待事件发生
    int rt = iom->addEvent(fd, dht::IOManager::WRITE);
    if(rt == 0) {
        // 切出当前协程，让出执行权给其他协程，等待超时或连接成功
        dht::Fiber::YieldToHold();
        if(timer) {
            timer->cancel();
        }
        // 如果 tinfo 被标记为取消，返回相应的错误码
        if(tinfo->cancelled) {
            errno = tinfo->cancelled;
            return -1;
        }
    } else {
        // 添加事件失败，取消定时器，返回错误信息
        if(timer) {
            timer->cancel();
        }
        DHT_LOG_ERROR(g_logger) << "connect addEvent(" << fd << ", WRITE) error";
    }
    // 检查连接的结果，获取 SO_ERROR 错误码，如果为 0 表示连接成功，否则表示连接失败
    int error = 0;
    socklen_t len = sizeof(int);
    if(-1 == getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &len)) {
        return -1;
    }
    if(!error) {
        return 0;
    } else {
        errno = error;
        return -1;
    }
}

int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
    return connect_with_timeout(sockfd, addr, addrlen, dht::s_connect_timeout);
}

int accept(int s, struct sockaddr *addr, socklen_t *addrlen) {
    int fd = do_io(s, accept_f, "accept", dht::IOManager::READ, SO_RCVTIMEO, addr, addrlen);
    if(fd >= 0) {
        dht::FdMgr::GetInstance()->get(fd, true);
    }
    return fd;
}
/**
 * read
 */

ssize_t read(int fd, void *buf, size_t count) {
    return do_io(fd, read_f, "read", dht::IOManager::READ, SO_RCVTIMEO, buf, count);
}

ssize_t readv(int fd, const struct iovec *iov, int iovcnt) {
    return do_io(fd, readv_f, "readv", dht::IOManager::READ, SO_RCVTIMEO, iov, iovcnt);
}

ssize_t recv(int sockfd, void *buf, size_t len, int flags) {
    return do_io(sockfd, recv_f, "recv", dht::IOManager::READ, SO_RCVTIMEO, buf, len, flags);
}

ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen) {
    return do_io(sockfd, recvfrom_f, "recvfrom", dht::IOManager::READ, SO_RCVTIMEO, buf, len, flags, src_addr, addrlen);
}

ssize_t recvmsg(int sockfd, struct msghdr *msg, int flags) {
    return do_io(sockfd, recvmsg_f, "recvmsg", dht::IOManager::READ, SO_RCVTIMEO, msg, flags);
}

/**
 * wirte
 */

ssize_t write(int fd, const void *buf, size_t count) {
    return do_io(fd, write_f, "write", dht::IOManager::WRITE, SO_SNDTIMEO, buf, count);
}

ssize_t writev(int fd, const struct iovec *iov, int iovcnt) {
    return do_io(fd, writev_f, "writev", dht::IOManager::WRITE, SO_SNDTIMEO, iov, iovcnt);
}

ssize_t send(int s, const void *msg, size_t len, int flags) {
    return do_io(s, send_f, "send", dht::IOManager::WRITE, SO_SNDTIMEO, msg, len, flags);
}

ssize_t sendto(int s, const void *msg, size_t len, int flags, const struct sockaddr *to, socklen_t tolen) {
    return do_io(s, sendto_f, "sendto", dht::IOManager::WRITE, SO_SNDTIMEO, msg, len, flags, to, tolen);
}

ssize_t sendmsg(int s, const struct msghdr *msg, int flags) {
    return do_io(s, sendmsg_f, "sendmsg", dht::IOManager::WRITE, SO_SNDTIMEO, msg, flags);
}

int close(int fd) {
    if(!dht::t_hook_enable) {
        return close_f(fd);
    }

    dht::FdCtx::ptr ctx = dht::FdMgr::GetInstance()->get(fd);
    if(ctx) {
        auto iom = dht::IOManager::GetThis();
        if(iom) {
            iom->cancelAll(fd);
        }
        dht::FdMgr::GetInstance()->del(fd);
    }
    return close_f(fd);
}

int fcntl(int fd, int cmd, ... /* arg */ ) {
    va_list va;
    va_start(va, cmd);
    switch(cmd) {
        case F_SETFL:
        {
            int arg = va_arg(va, int);
            va_end(va);
            dht::FdCtx::ptr ctx = dht::FdMgr::GetInstance()->get(fd);
            if(!ctx || ctx->isClose() || !ctx->isSocket()) {
                return fcntl_f(fd, cmd, arg);
            }
            ctx->setUserNonblock(arg & O_NONBLOCK);
            if(ctx->getSysNonblock()) {
                arg |= O_NONBLOCK;
            } else {
                arg &= ~O_NONBLOCK;
            }
            return fcntl_f(fd, cmd, arg);
        }
            break;
        case F_GETFL:
        {
            va_end(va);
            int arg = fcntl_f(fd, cmd);
            dht::FdCtx::ptr ctx = dht::FdMgr::GetInstance()->get(fd);
            if(!ctx || ctx->isClose() || !ctx->isSocket()) {
                return arg;
            }
            if(ctx->getUserNonblock()) {
                return arg | O_NONBLOCK;
            } else {
                return arg & ~O_NONBLOCK;
            }
        }
            break;
        case F_DUPFD:
        case F_DUPFD_CLOEXEC:
        case F_SETFD:
        case F_SETOWN:
        case F_SETSIG:
        case F_SETLEASE:
        case F_NOTIFY:
#ifdef F_SETPIPE_SZ
        case F_SETPIPE_SZ:
#endif
        {
            int arg = va_arg(va, int);
            va_end(va);
            return fcntl_f(fd, cmd, arg);
        }
            break;
        case F_GETFD:
        case F_GETOWN:
        case F_GETSIG:
        case F_GETLEASE:
#ifdef F_GETPIPE_SZ
        case F_GETPIPE_SZ:
#endif
        {
            va_end(va);
            return fcntl_f(fd, cmd);
        }
            break;
        case F_SETLK:
        case F_SETLKW:
        case F_GETLK:
        {
            struct flock* arg = va_arg(va, struct flock*);
            va_end(va);
            return fcntl_f(fd, cmd, arg);
        }
            break;
        case F_GETOWN_EX:
        case F_SETOWN_EX:
        {
            struct f_owner_exlock* arg = va_arg(va, struct f_owner_exlock*);
            va_end(va);
            return fcntl_f(fd, cmd, arg);
        }
            break;
        default:
            va_end(va);
            return fcntl_f(fd, cmd);
    }
}

int ioctl(int d, unsigned long int request, ...) {
    va_list va;
    va_start(va, request);
    void* arg = va_arg(va, void*);
    va_end(va);

    if(FIONBIO == request) {
        bool user_nonblock = !!*(int*)arg;
        dht::FdCtx::ptr ctx = dht::FdMgr::GetInstance()->get(d);
        if(!ctx || ctx->isClose() || !ctx->isSocket()) {
            return ioctl_f(d, request, arg);
        }
        ctx->setUserNonblock(user_nonblock);
    }
    return ioctl_f(d, request, arg);
}

int getsockopt(int sockfd, int level, int optname, void *optval, socklen_t *optlen) {
    return getsockopt_f(sockfd, level, optname, optval, optlen);
}

int setsockopt(int sockfd, int level, int optname, const void *optval, socklen_t optlen) {
    if(!dht::t_hook_enable) {
        return setsockopt_f(sockfd, level, optname, optval, optlen);
    }
    if(level == SOL_SOCKET) {
        if(optname == SO_RCVTIMEO || optname == SO_SNDTIMEO) {
            dht::FdCtx::ptr ctx = dht::FdMgr::GetInstance()->get(sockfd);
            if(ctx) {
                const timeval* v = (const timeval*)optval;
                ctx->setTimeout(optname, v->tv_sec * 1000 + v->tv_usec / 1000);
            }
        }
    }
    return setsockopt_f(sockfd, level, optname, optval, optlen);
}


}



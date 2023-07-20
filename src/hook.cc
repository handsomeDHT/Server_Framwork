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

//因为在执行main函数前，会先将所有全局变量的定义进行执行
//所以可以利用这个特性，定义一些在main函数之前就会执行的方法
struct _HookIniter {
    _HookIniter() {
        hook_init();
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

template<typename OriginFun, typename... Args>
static ssize_t do_io(int fd, OriginFun fun, const char* hook_fun_name
        , uint32_t event, int timeout_so, Args&&... args){
    if(dht::t_hook_enable){
        return fun(fd, std::forward<Args>(args)...);
    }

    dht::FdCtx::ptr ctx = dht::FdMgr::GetInstance()->get(fd);
    if(!ctx){
        return fun(fd, std::forward<Args>(args)...);
    }
    if(ctx->isClosed()){
        errno = EBADF;
        return -1;
    }
    if(!ctx->isSocket() || ctx->getUserNonblock()) {
        return fun(fd, std::forward<args>(args)...);
    }

    uint64_t to = ctx->getTimeout(timeout_so);
    std::shared_ptr<timer_info> tinfo(new timer_info);

retry:
    ssize_t n = fun(fd, std::forward<Args>(args)...);
    while (n == -1 && errno == EINTR) {
        n = fun(fd, std::forward<Args>(args)...);
    }
    if(n == -1 && errno == EAGAIN) {
        dht::IOManager* iom = dht::IOManager::GetThis();
        dht::Timer::ptr timer;
        std::weak_ptr<timer_info> winfo(tinfo);

        if(to != (uint64_t)-1){
            timer = iom->addConditionTime(to, [winfo, fd, iom, event](){
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
        int rt = iom->addEvent(fd, (dht::IOManager::Event)(event));
        if(rt){
                DHT_LOG_ERROR(g_logger) << hook_fun_name << " addEvent("
                                        << fd << ", " << event << ")";
            if(timer) {
                timer->cancel();
            }
            return -1;
        }else {
            dht::Fiber::YieldToHold();
            if(timer) {
                timer->cancel();
            }
            if(tinfo->cancelled) {
                errno = tinfo->cancelled;
                return -1;
            }
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
    //iom->addTimer(seconds * 1000, std::bind(&dht::IOManager::schedule, iom, fiber));
    iom->addTimer(seconds * 1000, [iom, fiber](){
        iom->schedule(fiber);
    });
    dht::Fiber::YieldToHold();
    return 0;
}

int usleep(useconds_t usec){
    if(!dht::t_hook_enable){
        return usleep_f(usec);
    }
    dht::Fiber::ptr fiber = dht::Fiber::GetThis();
    dht::IOManager* iom = dht::IOManager::GetThis();
    //iom->addTimer(usec / 1000, std::bind(&dht::IOManager::schedule, iom, fiber));
    iom->addTimer(usec / 1000, [iom, fiber](){
        iom->schedule(fiber);
    });
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
    //iom->addTimer(usec / 1000, std::bind(&dht::IOManager::schedule, iom, fiber));
    iom->addTimer(timeout_ms, [iom, fiber](){
        iom->schedule(fiber);
    });
    dht::Fiber::YieldToHold();
    return 0;
}

/**
 * socket API
 */

/**
 * read
 */

/**
 * wirte
 */


}



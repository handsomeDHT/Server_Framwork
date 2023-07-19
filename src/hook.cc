//
// Created by 帅帅的涛 on 2023/7/18.
//

#include "hook.h"
#include "fiber.h"
#include "iomanager.h"
#include "functional"
#include <dlfcn.h>

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



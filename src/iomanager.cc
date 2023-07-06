//
// Created by 帅帅的涛 on 2023/7/6.
//

#include "iomanager.h"
#include "macro.h"
#include "log.h"
#include <unistd.h>
#include <sys/epoll.h>
#include <fcntl.h>

namespace dht{

static dht::Logger::ptr g_logger = DHT_LOG_NAME("system");

IOManager::IOManager(size_t threads, bool use_caller, const std::string &name)
        : Scheduler(threads, use_caller, name){
    m_epfd = epoll_create(5000);
    DHT_ASSERT(m_epfd > 0);

    int rt = pipe(m_tickleFds);
    DHT_ASSERT(rt);

    epoll_event event;
    memset(&event, 0, sizeof(epoll_event));
    event.events = EPOLLIN | EPOLLET;
    event.data.fd = m_tickleFds[0];

    rt = fcntl(m_tickleFds[0], F_SETFL, O_NONBLOCK);
    DHT_ASSERT(rt);

    rt = epoll_ctl(m_epfd, EPOLL_CTL_ADD, m_tickleFds[0], &event);
    DHT_ASSERT(rt);

    contextResize(32);

    start();
}

IOManager::~IOManager() noexcept {
    stop();
    close(m_epfd);
    close(m_tickleFds[0]);
    close(m_tickleFds[1]);

    for(size_t i = 0; i < m_fdContexts.size(); ++i){
        if(m_fdContexts[i]){
            delete m_fdContexts[i];
        }
    }
}

void IOManager::contextResize(size_t size) {
    m_fdContexts.resize(size);

    for(size_t i = 0; i < m_fdContexts.size(); ++i){
        if(!m_fdContexts[i]){
            m_fdContexts[i] = new FdContext;
            m_fdContexts[i]->fd = i;
        }
    }
}

int IOManager::addEvent(int fd, Event event, std::function<void()> cb) {
    FdContext* fd_ctx = nullptr;
    RWMUtexType::ReadLock lock(m_mutex);
    if(m_fdContexts.size() > fd){
        fd_ctx = m_fdContexts[fd];
        lock.unlock();
    }else{
        lock.unlock();
        RWMUtexType::WriteLock lock2(m_mutex);
        contextResize(m_fdContexts.size() * 1.5);
        fd_ctx = m_fdContexts[fd];
    }

    FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    if(fd_ctx->m_events & event) {
        //DHT_LOG_ERROR(g_logger) << ""
    }
}

bool IOManager::delEvent(int fd, Event event) {

}

bool IOManager::cancelEvent(int fd, Event event) {

}

bool IOManager::cancelAll(int fd) {

}

IOManager *IOManager::GetThis() {

}



}


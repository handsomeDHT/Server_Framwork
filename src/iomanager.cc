#include "iomanager.h"
#include "macro.h"
#include "log.h"

#include <unistd.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

namespace dht{

static dht::Logger::ptr g_logger = DHT_LOG_NAME("system");

enum EpollCtlOp {
};

static std::ostream& operator<< (std::ostream& os, const EpollCtlOp& op) {
    switch((int)op) {
#define XX(ctl) \
        case ctl: \
            return os << #ctl;
        XX(EPOLL_CTL_ADD);
        XX(EPOLL_CTL_MOD);
        XX(EPOLL_CTL_DEL);
        default:
            return os << (int)op;
    }
#undef XX
}

static std::ostream& operator<< (std::ostream& os, EPOLL_EVENTS events) {
    if(!events) {
        return os << "0";
    }
    bool first = true;
#define XX(E) \
    if(events & E) { \
        if(!first) { \
            os << "|"; \
        } \
        os << #E; \
        first = false; \
    }
    XX(EPOLLIN);
    XX(EPOLLPRI);
    XX(EPOLLOUT);
    XX(EPOLLRDNORM);
    XX(EPOLLRDBAND);
    XX(EPOLLWRNORM);
    XX(EPOLLWRBAND);
    XX(EPOLLMSG);
    XX(EPOLLERR);
    XX(EPOLLHUP);
    XX(EPOLLRDHUP);
    XX(EPOLLONESHOT);
    XX(EPOLLET);
#undef XX
    return os;
}

IOManager::FdContext::EventContext &IOManager::FdContext::getContext(IOManager::Event event) {
    switch(event) {
        case IOManager::READ:
            return read;
        case IOManager::WRITE:
            return write;
        default:
            DHT_ASSERT2(false, "getContext");
    }
    throw std::invalid_argument("getContext invalid event");
}

void IOManager::FdContext::resetContext(EventContext &ctx) {
    ctx.scheduler = nullptr;
    ctx.fiber.reset();
    ctx.cb = nullptr;
}

void IOManager::FdContext::triggerEvent(Event event) {
    DHT_ASSERT(events & event);
    events = (Event)(events & ~event);
    EventContext& ctx = getContext(event);
    if(ctx.cb) {
        ctx.scheduler->schedule(&ctx.cb);
    } else {
        ctx.scheduler->schedule(&ctx.fiber);
    }
    ctx.scheduler = nullptr;
    return;
}


IOManager::IOManager(size_t threads, bool use_caller, const std::string &name)
        : Scheduler(threads, use_caller, name){

    DHT_LOG_INFO(g_logger) << "Init IOManager，创建管道并设置epoll_event事件类型";
    /**
     * @brief 创建一个epoll实例
     * size制定了epoll实例能监听的最大文件描述符数量
     * 该参数可以被忽略，一般取一个很大的数确保可以容纳需要监听的文件描述符数量
     *
     * @return 返回一个整数值，调用成功->返回一个>=0的数
     *                      调用失败->返回-1
     */

    m_epfd = epoll_create(5000);

    DHT_ASSERT(m_epfd > 0);

    int rt = pipe(m_tickleFds);
    DHT_ASSERT(!rt);

    epoll_event event;
    /**
     * memset -> 内存初始化
     * 将event变量的内存区域初始化为0，以准备填充"epoll_event"结构体的各个成员
     */
    memset(&event, 0, sizeof(epoll_event));
    //设置epoll_event的事件类型为EPOLLIN(可读事件)和EPOLLET(边缘触发模式)
    event.events = EPOLLIN | EPOLLET;
    //将管道读取端信息作为event的文件描述符
    event.data.fd = m_tickleFds[0];
    //对文件输出端m_tickleFds[0]设置(F_SETFL)为非阻塞模式(O_NONBLOCK)
    //F_SETFL是一个控制命令，用来指定所执行的操作类型
    rt = fcntl(m_tickleFds[0], F_SETFL, O_NONBLOCK);
    DHT_ASSERT(!rt);

    /**
     * epoll_ctl是一个用于控制epoll实例中事件的系统调用
     * 这段代码使用 epoll_ctl 函数将文件描述符 m_tickleFds[0] 添加到 epoll 实例中，以便监视该文件描述符上的事件。
     */
    rt = epoll_ctl(m_epfd, EPOLL_CTL_ADD, m_tickleFds[0], &event);
    DHT_ASSERT(!rt);

    contextResize(32);

    start();
}

IOManager::~IOManager(){
    DHT_LOG_INFO(g_logger) << "将任务配置到iom中后，在析构函数阶段，执行stop并进行join等待任务执行完成";
    stop();//在生命周结束时，执行所有未执行的Thread
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
    if((int)m_fdContexts.size() > fd){
        //说明之前已经为该文件描述符分配了相应的FdContext对象
        //可以直接通过索引fd获取该定向并赋值给fd_ctx
        fd_ctx = m_fdContexts[fd];
        lock.unlock();
    }else{
        lock.unlock();
        RWMUtexType::WriteLock lock2(m_mutex);
        contextResize(fd * 1.5);
        fd_ctx = m_fdContexts[fd];
    }

    FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    if(DHT_UNLIKELY(fd_ctx->events & event)) {
        DHT_LOG_ERROR(g_logger) << "addEvent assert fd=" << fd
                                << " event=" << (EPOLL_EVENTS)event
                                << " fd_ctx.event=" << (EPOLL_EVENTS)fd_ctx->events;
        DHT_ASSERT(!(fd_ctx->events & event));
    }
    //EPOLL_CTL_MOD->修改
    //EPOLL_CTL_ADD->添加
    int op = fd_ctx->events ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;
    epoll_event epevent;
    epevent.events = EPOLLET | fd_ctx->events | event;
    epevent.data.ptr = fd_ctx;

    /**
     * @param[op] 操作类型
     * @param[fd] 进行操作的文件描述符
     */
    int rt = epoll_ctl(m_epfd, op, fd, &epevent);
    if(rt){
        DHT_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
                                << (EpollCtlOp)op << ", " << fd << ", " << (EPOLL_EVENTS)epevent.events << "):"
                                << rt << " (" << errno << ") (" << strerror(errno) << ") fd_ctx->events="
                                << (EPOLL_EVENTS)fd_ctx->events;
        return -1;
    }

    ++m_pendingEventCount;
    //将新添加的事件类型合并到已有的事件类型中
    fd_ctx->events = (Event)(fd_ctx->events | event);
    FdContext::EventContext& event_ctx = fd_ctx->getContext(event);
    DHT_ASSERT(!event_ctx.scheduler
                    && !event_ctx.fiber
                    && !event_ctx.cb);
    event_ctx.scheduler = Scheduler::GetThis();
    if(cb){
        event_ctx.cb.swap(cb);
    } else {
        event_ctx.fiber = Fiber::GetThis();
        DHT_ASSERT2(event_ctx.fiber->getState() == Fiber::EXEC
            ,"state=" << event_ctx.fiber->getState());
    }
    return 0;
}

bool IOManager::delEvent(int fd, Event event) {
    RWMUtexType::ReadLock lock(m_mutex);
    //检查fd索引是否合法
    if((int)m_fdContexts.size() <= fd) {
        return false;
    }
    FdContext* fd_ctx = m_fdContexts[fd];
    lock.unlock();

    FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    if(DHT_UNLIKELY(!(fd_ctx->events & event))){
        return false;
    }
    //更新fd_ctx的events，将指定的event从events中删除
    Event new_events = (Event)(fd_ctx->events & ~event);
    int op = new_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;

    epoll_event epevent;
    epevent.events = EPOLLET | new_events; //EPOLLET：边缘触发模式
    epevent.data.ptr = fd_ctx;
    int rt = epoll_ctl(m_epfd, op, fd, &epevent);
    if(rt) {
        DHT_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
                                << (EpollCtlOp)op << ", " << fd << ", " << (EPOLL_EVENTS)epevent.events << "):"
                                << rt << " (" << errno << ") (" << strerror(errno) << ")";
        return false;
    }

    --m_pendingEventCount;
    //更新fd_ctx(m_fdContexts[fd])的events
    fd_ctx->events = new_events;

    //重置对应event的EventContext，以清除相关数据
    FdContext::EventContext& event_ctx = fd_ctx->getContext(event);
    fd_ctx->resetContext(event_ctx);
    return true;
}

bool IOManager::cancelEvent(int fd, Event event) {
    RWMUtexType::ReadLock lock(m_mutex);
    if((int)m_fdContexts.size() <= fd) {
        return false;
    }
    FdContext* fd_ctx = m_fdContexts[fd];
    lock.unlock();

    FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    if(DHT_UNLIKELY(!(fd_ctx->events & event))){
        return false;
    }
    Event new_events = (Event)(fd_ctx->events & ~event);
    int op = new_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
    epoll_event epevent;
    epevent.events = EPOLLET | new_events;
    epevent.data.ptr = fd_ctx;
    int rt = epoll_ctl(m_epfd, op, fd, &epevent);
    if(rt) {
        DHT_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
                                  << (EpollCtlOp)op << ", " << fd << ", " << (EPOLL_EVENTS)epevent.events << "):"
                                  << rt << " (" << errno << ") (" << strerror(errno) << ")";
        return false;
    }
    //FdContext::EventContext& event_ctx = fd_ctx->getContext(event);
    fd_ctx->triggerEvent(event);
    --m_pendingEventCount;
    return true;
}

bool IOManager::cancelAll(int fd) {
    RWMUtexType::ReadLock lock(m_mutex);
    if((int)m_fdContexts.size() <= fd) {
        return false;
    }
    FdContext* fd_ctx = m_fdContexts[fd];
    lock.unlock();

    FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    if(!fd_ctx->events){
        return false;
    }

    int op = EPOLL_CTL_DEL;
    epoll_event epevent;
    epevent.events = 0;
    epevent.data.ptr = fd_ctx;

    int rt = epoll_ctl(m_epfd, op, fd, &epevent);
    if(rt) {
        DHT_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
                                << (EpollCtlOp)op << ", " << fd << ", " << (EPOLL_EVENTS)epevent.events << "):"
                                << rt << " (" << errno << ") (" << strerror(errno) << ")";
        return false;
    }

    if(fd_ctx->events & READ){
        fd_ctx->triggerEvent(READ);
        --m_pendingEventCount;
    }
    if(fd_ctx->events & WRITE){
        fd_ctx->triggerEvent(WRITE);
        --m_pendingEventCount;
    }

    DHT_ASSERT(fd_ctx->events == 0);

    return true;
}

IOManager* IOManager::GetThis() {
    return dynamic_cast<IOManager*>(Scheduler::GetThis());
}

void IOManager::tickle() {
    if(!hasIdleThreads()){
        return;
    }
    /**
     * ssize_t write (int __fd, const void *__buf, size_t __n) __wur;
     * @param[fd] 写入数据的文件描述符
     * @param[buf] 要写入数据的缓冲区指针
     * @para,[n] 要写入的字节数
     */

    int rt = write(m_tickleFds[1], "T", 1);
    DHT_ASSERT(rt == 1);
}

bool IOManager::stopping(uint64_t &timeout) {
    timeout = getNextTimer();
    return timeout == ~0ull
            && m_pendingEventCount == 0
            && Scheduler::stopping();
}

bool IOManager::stopping() {
    uint64_t timeout = 0;
    return stopping(timeout);
}

void IOManager::idle() {

    DHT_LOG_DEBUG(g_logger) << "idle";
    const uint64_t MAX_EVNETS = 256;

    epoll_event* events = new epoll_event[MAX_EVNETS]();
    //使用智能指针管理events数组的内存释放
    std::shared_ptr<epoll_event> shared_events(events, [](epoll_event* ptr){
        delete[] ptr;
    });

    while(true) {
        uint64_t next_timeout = 0;
        if(DHT_UNLIKELY(stopping(next_timeout))) {
            DHT_LOG_INFO(g_logger) << "name=" << getName()
                                     << " idle stopping exit";
            break;
        }

        int rt = 0;
        do {
            static const int MAX_TIMEOUT = 3000;
            if(next_timeout != ~0ull) {
                next_timeout = (int)next_timeout > MAX_TIMEOUT
                               ? MAX_TIMEOUT : next_timeout;
            } else {
                next_timeout = MAX_TIMEOUT;
            }
            rt = epoll_wait(m_epfd, events, MAX_EVNETS, (int)next_timeout);
            if(rt < 0 && errno == EINTR) {
            } else {
                break;
            }
        } while(true);

        std::vector<std::function<void()> > cbs;
        listExpiredCb(cbs);
        if(!cbs.empty()) {
            //SYLAR_LOG_DEBUG(g_logger) << "on timer cbs.size=" << cbs.size();
            schedule(cbs.begin(), cbs.end());
            cbs.clear();
        }

        //if(SYLAR_UNLIKELY(rt == MAX_EVNETS)) {
        //    SYLAR_LOG_INFO(g_logger) << "epoll wait events=" << rt;
        //}

        for(int i = 0; i < rt; ++i) {
            epoll_event& event = events[i];
            if(event.data.fd == m_tickleFds[0]) {
                uint8_t dummy[256];
                while(read(m_tickleFds[0], dummy, sizeof(dummy)) > 0);
                continue;
            }

            FdContext* fd_ctx = (FdContext*)event.data.ptr;
            FdContext::MutexType::Lock lock(fd_ctx->mutex);
            if(event.events & (EPOLLERR | EPOLLHUP)) {
                event.events |= (EPOLLIN | EPOLLOUT) & fd_ctx->events;
            }
            int real_events = NONE;
            if(event.events & EPOLLIN) {
                real_events |= READ;
            }
            if(event.events & EPOLLOUT) {
                real_events |= WRITE;
            }

            if((fd_ctx->events & real_events) == NONE) {
                continue;
            }

            int left_events = (fd_ctx->events & ~real_events);
            int op = left_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
            event.events = EPOLLET | left_events;

            int rt2 = epoll_ctl(m_epfd, op, fd_ctx->fd, &event);
            if(rt2) {
                DHT_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
                                          << (EpollCtlOp)op << ", " << fd_ctx->fd << ", " << (EPOLL_EVENTS)event.events << "):"
                                          << rt2 << " (" << errno << ") (" << strerror(errno) << ")";
                continue;
            }

            //SYLAR_LOG_INFO(g_logger) << " fd=" << fd_ctx->fd << " events=" << fd_ctx->events
            //                         << " real_events=" << real_events;
            if(real_events & READ) {
                fd_ctx->triggerEvent(READ);
                --m_pendingEventCount;
            }
            if(real_events & WRITE) {
                fd_ctx->triggerEvent(WRITE);
                --m_pendingEventCount;
            }
        }

        Fiber::ptr cur = Fiber::GetThis();
        auto raw_ptr = cur.get();
        cur.reset();

        raw_ptr->swapOut();
    }
}

void IOManager::onTimerInsertedAtFront() {
    tickle();
}

}


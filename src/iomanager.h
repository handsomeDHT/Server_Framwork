/**
 * 基于Epoll的IO携程调度器
 */


#ifndef SERVER_FRAMWORK_IOMANAGER_H
#define SERVER_FRAMWORK_IOMANAGER_H

#include "scheduler.h"

namespace dht{

class IOManager : public Scheduler{
public:
    typedef std::shared_ptr<IOManager> ptr;
    typedef RWMutex RWMUtexType;

    //IO事件
    enum Event {
        //无事件
        NONE = 0x0,
        //读事件(EPOLLIN)
        READ = 0x1,
        //写事件(EPOLLOUT)
        WRITE = 0x2
    };
private:
    //Socket事件上线文类
    struct FdContext{
        typedef Mutex MutexType;
        //事件上线文类
        struct EventContext{
            Scheduler* scheduler = nullptr;     //待执行的scheduler
            Fiber::ptr fiber;         //事件协程
            std::function<void()> cb; //事件的回调函数
        };

        /**
         * @brief 获取事件上下文类
         * @param[in] event 事件类型
         * @return 返回对应事件的上线文
         */
        EventContext& getContext(Event event);
        //重置事件上下文(ctx->待重置的上下文类)
        void resetContext(EventContext& ctx);
        //触发事件
        void triggerEvent(Event event);

        EventContext read;//读事件
        EventContext write;//写事件
        int fd = 0;        //事件关联的句柄
        Event events = NONE;//已经注册的事件
        MutexType mutex;
    };

public:
    IOManager(size_t threads = 1
                ,bool use_caller = true
                ,const std::string& name = "");
    ~IOManager();

    /**
     * @brief 添加事件
     * @param[in] fd socket句柄
     * @param[in] event 事件类型
     * @param[in] cb 事件回调函数
     * @return 添加成功返回0,失败返回-1
     */
    int addEvent(int fd, Event event, std::function<void()> cb = nullptr);
    bool delEvent(int fd, Event event);

    bool cancelEvent(int fd, Event event);
    bool cancelAll(int fd);

    static IOManager* GetThis();

protected:
    void tickle() override;
    bool stopping() override;
    void idle() override;

    void contextResize(size_t size);
private:
    //epoll 文件句柄
    int m_epfd = 0;
    // pipe文件句柄
    int m_tickleFds[2];
    //当前等待执行的事件数量
    std::atomic<size_t> m_pendingEventCount = {0};
    RWMUtexType m_mutex;
    //socket事件上下文的容器
    std::vector<FdContext*> m_fdContexts;
};

}

#endif //SERVER_FRAMWORK_IOMANAGER_H

//
// Created by 帅帅的涛 on 2023/6/20.
//

#include "scheduler.h"
#include "log.h"
#include "macro.h"


namespace dht{
static dht::Logger::ptr g_logger = DHT_LOG_NAME("system");

static thread_local Scheduler* t_scheduler = nullptr;
static thread_local Fiber* t_fiber = nullptr; //主协程

Scheduler::Scheduler(size_t threads, bool use_caller, const std::string &name)
    :m_name(name){
    DHT_ASSERT(threads > 0);

    if(use_caller){
        dht::Fiber::GetThis();
        --threads;

        DHT_ASSERT(GetThis() == nullptr);
        t_scheduler = this;

        m_rootFiber.reset(new Fiber(std::bind(&Scheduler::run, this) ,0, true));
        dht::Thread::SetName(m_name);

        t_fiber = m_rootFiber.get();
        m_rootThread = dht::GetThreadId();
        m_threadIds.push_back(m_rootThread);
    }else {
        m_rootThread = -1;
    }
    //初始化阶段配置线程数量
    m_threadCount = threads;
}

Scheduler::~Scheduler() {
    DHT_ASSERT(m_stopping);
    if(GetThis() == this){
        t_scheduler = nullptr;
    }
}

Scheduler *Scheduler::GetThis() {
    return t_scheduler;
}

Fiber *Scheduler::GetMainFiber() {
    return t_fiber;
}

void Scheduler::start() {
    MutexType::Lock lock(m_mutex);
    if(!m_stopping){
        return;
    }
    m_stopping = false;
    DHT_ASSERT(m_threads.empty());

    m_threads.resize(m_threadCount);
    for(size_t i = 0; i < m_threadCount; ++i){
        //初始化对应数量的线程
        m_threads[i].reset(new Thread(std::bind(&Scheduler::run, this)
                                      , m_name + "_" + std::to_string(i)));
        //各线程对应ID
        m_threadIds.push_back(m_threads[i]->getId());
    }
    lock.unlock();

}

void Scheduler::stop() {
    m_autoStop = true;
    if(m_rootFiber
            && m_threadCount == 0
            && ((m_rootFiber->getState() == Fiber::TERM)
                || m_rootFiber -> getState() == Fiber::INIT)){
        DHT_LOG_INFO(g_logger) << this << " stopped";
        m_stopping = true;

        if(stopping()) {
            return;
        }
    }

    if(m_rootThread != -1){
        DHT_ASSERT(GetThis() == this);
    }else{
        DHT_ASSERT(GetThis() != this);
    }

    m_stopping = true;
    for(size_t i = 0; i < m_threadCount; ++i){
        tickle();
    }

    if(m_rootFiber){
        //如果存在协程任务，通知
        tickle();
    }

    if(m_rootFiber){
        if(!stopping()) {
            //任务执行，将当前任务切换到执行状态
            m_rootFiber->call();
        }
    }
    std::vector<Thread::ptr> thrs;
    {
        MutexType::Lock lock(m_mutex);
        //将m_threads线程池复制出来
        thrs.swap(m_threads);
    }

    for(auto& i : thrs){
        i->join();//等待线程执行完成
    }
}

void Scheduler::run() {
    setThis();
    //如果当前ID不等于主线程的ID,将当前线程重置成自己
    if(dht::GetThreadId() != m_rootThread){

        t_fiber = Fiber::GetThis().get();
    }
    //当线程没事做的时候，用Idle
    Fiber::ptr idle_fiber(new Fiber(std::bind(&Scheduler::idle, this)));
    Fiber::ptr cb_fiber;

    FiberAndThread ft;
    while(true){
        ft.reset();
        bool tickle_me = false;
        bool is_active = false;
        //从消息队列中取出一个需要执行的消息
        {
            MutexType::Lock lock(m_mutex);
            auto it = m_fibers.begin();
            while(it != m_fibers.end()) {
                //如果当前任务不属于当前线程，那么不执行，并通知其他线程去执行任务
                if(it->thread != -1 && it->thread != dht::GetThreadId()){
                    ++it;
                    tickle_me = true;//通知其他线程唤醒去执行任务
                    continue;
                }
                DHT_ASSERT(it -> fiber || it -> cb);
                //如果这个线程的状态是EXEC(正在执行状态)，那么也不进行操作
                if(it->fiber && it->fiber->getState() == Fiber::EXEC ){
                    ++it;
                    continue;
                }
                //以上两种状态都不满足，则把该任务赋值给ft,并从任务队列中将他抹除
                ft = *it;
                m_fibers.erase(it);
                ++m_activeThreadCount;
                is_active = true;
                break;
            }
        }
        //如果存在其他线程的任务，通知一下
        if(tickle_me){
            tickle();
        }

        //执行拿到的线程，其中包含一个协程对象
        //两种任务，一个是fiber,一个是cb
        if(ft.fiber && (ft.fiber->getState() != Fiber::TERM
                    || ft.fiber->getState() != Fiber::EXCEPT) ){

            ft.fiber->swapIn();
            --m_activeThreadCount;

            if(ft.fiber->getState() == Fiber::READY){
                schedule(ft.fiber);
            }else if(ft.fiber->getState() != Fiber::TERM
                        && ft.fiber->getState() != Fiber::EXCEPT){
                ft.fiber->m_state = Fiber::HOLD;
            }
            ft.reset();
        }else if (ft.cb) {
            if(cb_fiber) {
                cb_fiber->reset(ft.cb);
            } else {
                cb_fiber.reset(new Fiber(ft.cb));
                ft.cb = nullptr;
            }
            ft.reset();
            --m_activeThreadCount;
            cb_fiber->swapIn();
            if(cb_fiber->getState() == Fiber::READY){
                schedule(cb_fiber);
                cb_fiber.reset();
            } else if(cb_fiber->getState() == Fiber::EXCEPT
                    || cb_fiber->getState() == Fiber::TERM){
                cb_fiber ->reset(nullptr);
            } else {
                cb_fiber -> m_state = Fiber::HOLD;
                cb_fiber.reset();
            }
        }else {
            if(is_active){
                --m_activeThreadCount;
                continue;
            }
            //两种任务都为空，说明已经执行完了，此时用idle来进行执行
            if(idle_fiber->getState() == Fiber::TERM) {
                DHT_LOG_INFO(g_logger) << "idle fiber term";
                //tickle();
                break;
            }

            ++m_idleThreadCount;
            idle_fiber->swapIn();
            --m_idleThreadCount;
            if(idle_fiber->getState() != Fiber::TERM
                        && idle_fiber->getState() != Fiber::EXCEPT){
                idle_fiber->m_state = Fiber::HOLD;
            }

        }
    }
}

bool Scheduler::stopping() {
    MutexType::Lock lock(m_mutex);
    return m_autoStop && m_stopping
           && m_fibers.empty() && m_activeThreadCount == 0;
}

void Scheduler::setThis() {
    t_scheduler = this;
}

void Scheduler::tickle() {
    DHT_LOG_INFO(g_logger) << "tickle";
}

void Scheduler::idle() {
    DHT_LOG_INFO(g_logger) << "idle";
    while(!stopping()){
        dht::Fiber::YieldToHold();
    }
}


}
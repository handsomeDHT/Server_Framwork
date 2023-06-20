//协程调度器

#ifndef SERVER_FRAMWORK_SCHEDULER_H
#define SERVER_FRAMWORK_SCHEDULER_H

#include "dht.h"
#include <memory>


namespace dht{
/**
 * @brief 协程调度器
 * @details 封装的是N-M的协程调度器
 *          内部有一个线程池,支持协程在线程池里面切换
 */
class Scheduler{
public:
    typedef std::shared_ptr<Scheduler> ptr;
    typedef Mutex MutexType;

    /**
     * @brief 构造函数
     * @param[in] threads 线程数量
     * @param[in] use_caller 是否使用当前调用线程
     * @param[in] name 协程调度器名称
     */
    Scheduler(size_t threads = 1
                ,bool use_caller = true
                ,const std::string& name = "");

    virtual ~Scheduler();

    const std::string& getName() const { return m_name; }
    static Scheduler* GetThis();
    /**
     * @brief 返回当前协程调度器的调度协程
     */
    static Fiber* GetMainFiber();

    void start();//启动协程调度器
    void stop();//停止协程调度器

    /**
     * @brief 调度协程
     * @param[in] fc 协程或函数
     * @param[in] thread 协程执行的线程id,-1标识任意线程
     */
    template<class FiberOrCb>
    void schedule(FiberOrCb fc, int thread = -1){
        bool need_tickle = false;
        {
            MutexType::Lock lock(m_mutex);
            need_tickle = scheduleNoLock(fc, thread);
        }

        if(need_tickle){
            tickle();
        }
    }

    /**
     * @brief 批量调度协程
     * @param[in] begin 协程数组的开始
     * @param[in] end 协程数组的结束
     */
    template<class InputIterator>
    void schedule(InputIterator begin, InputIterator end){
        bool need_tickle = false;
        {
            MutexType::Lock lock(m_mutex);
            while(begin != end ){
                need_tickle = scheduleNoLock(&*begin) || need_tickle;
            }
        }
        if(need_tickle) {
            tickle();
        }
    }
protected:
    virtual void tickle(); //通知协程调度器有任务了
private:
    /**
     * @brief 协程调度启动(无锁)
     */
    template<class FiberOrCb>
    bool scheduleNoLock(FiberOrCb fc, int thread){
        bool need_tickle = m_fibers.empty();
        FiberAndThread ft(fc, thread);
        if(ft.fiber || ft.cb){
            m_fibers.push_back(ft);
        }
        return need_tickle;
    }
private:
    /**
     * @brief 协程/函数/线程组
     */
    struct FiberAndThread{
        Fiber::ptr fiber; //协程
        std::function<void()> cb; //协程执行函数
        int thread; //线程id

        /**
         * @brief 构造函数
         * @param[in] f 协程
         * @param[in] thr 线程id
         */
        FiberAndThread(Fiber::ptr f, int thr)
            :fiber(f)
            ,thread(thr){
        }

        /**
         * @brief 构造函数
         * @param[in] f 协程指针
         * @param[in] thr 线程id
         * @post *f = nullptr
         */
        FiberAndThread(Fiber::ptr* f, int thr)
            :thread(thr){
            fiber.swap(*f);//通过将swap将传入的fiber置空，使其引用指数-1
        }

        /**
         * @brief 构造函数
         * @param[in] f 协程执行函数
         * @param[in] thr 线程id
         */
        FiberAndThread(std::function<void()> f, int thr)
            :cb(f), thread(thr){
        }

        /**
         * @brief 构造函数
         * @param[in] f 协程执行函数指针
         * @param[in] thr 线程id
         * @post *f = nullptr
         */
        FiberAndThread(std::function<void()>* f, int thr)
                :thread(thr){
            cb.swap(*f);
        }

        /**
         * @brief 无参构造函数
         */
        FiberAndThread()
            :thread(-1){
        }

        void reset (){
            fiber  = nullptr;
            cb = nullptr;
            thread = -1;
        }
    };
private:
    MutexType m_mutex;
    std::vector<Thread::ptr> m_threads;
    std::list<FiberAndThread> m_fibers;//即将要执行的、计划要执行的协程
    std::string m_name;
};
}

#endif //SERVER_FRAMWORK_SCHEDULER_H

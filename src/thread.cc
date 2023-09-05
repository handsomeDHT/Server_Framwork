//
// Created by 帅帅的涛 on 2023/5/18.
//
#include "thread.h"
#include "util.h"
#include "log.h"

namespace dht{

//线程局部变量，又来指向当前的线程信息
static thread_local Thread* t_thread = nullptr;
static thread_local std::string t_thread_name = "UNKNOW";
//将系统日志统一输出到"system"中去
static dht::Logger::ptr g_logger = DHT_LOG_NAME("system");

Thread *Thread::GetThis() {
    return t_thread;
}

const std::string &Thread::GetName() {
    return t_thread_name;
}

void Thread::SetName(const std::string &name) {
    if(t_thread){
        t_thread->m_name = name;
    }
    t_thread_name = name;
}

Thread::Thread(std::function<void()> cb, const std::string &name)
    :m_cb(cb)
    ,m_name(name) {

    if(name.empty()){
        m_name = "UNKNOW";
    }
    /**
     * int pthread_create(pthread_t* thread, const pthread_attr_t* attr,
     *                    void* (*start_routine)(void*), void* arg);
     * @param[thread]  指向 pthread_t 类型的指针，用于存储新创建的线程的标识符。
     * @param[attr]   指向 pthread_attr_t 类型的指针，用于指定新线程的属性。如果为 NULL，表示使用默认属性。
     * @param[start_routine]  线程要执行的函数，它是一个函数指针，接受一个 void* 类型的参数，并返回 void* 类型的结果。
     *                        在新线程中，该函数将作为线程的入口点执行。
     * @param[arg]  传递给 start_routine 函数的参数，它是一个 void* 类型的指针。
     *
     * pthread_create 函数用于创建一个新的线程，新线程会执行 start_routine 函数，并将 arg 作为其参数。
     * 新线程的标识符将存储在 thread 指向的内存位置中。
     *
     * @return 0:创建成功
     * 创建失败翻译一个非零错误码：
     *      EAGAIN：表示资源不足，无法创建新的线程。
     *      EINVAL：表示传递给函数的参数无效，如传递了不支持的线程属性。
     *      EPERM：表示权限不足，无法创建线程
     */
    int rt = pthread_create(&m_thread, nullptr, &Thread::run, this);
    if(rt){
        DHT_LOG_ERROR(g_logger) << "pthread_create thread failed, rt=" << rt
            << " name=" << name;
        throw std::logic_error("pthread_create error");
    }
    m_semaphore.wait();
}

Thread::~Thread() {
    if(m_thread){
        pthread_detach(m_thread);
    }
}

//pthread_join 函数将会阻塞当前线程，直到指定的线程（由 thread 参数指定）执行结束。
//一旦线程结束，pthread_join 将会获取该线程的返回值，并将其存储在 value_ptr(thread_return) 指向的内存位置中。
/**
 * int pthread_join(pthread_t thread, void **retval)
 * @param[thread] 被连接线程的线程号
 * @param[retval] 指向一个指向被连接线程的返回码的指针的指针
 */
void Thread::join() {
    if(m_thread) {
        int rt = pthread_join(m_thread, nullptr);
        if(rt) {
            DHT_LOG_ERROR(g_logger) << "pthread_join thread fail, rt=" << rt
                                      << " name=" << m_name;
            throw std::logic_error("pthread_join error");
        }
        m_thread = 0;
    }
}

void* Thread::run(void* arg) {
    Thread* thread = (Thread*)arg;
    t_thread = thread;
    t_thread_name = thread->m_name;
    thread->m_id = dht::GetThreadId();
    /**
     * pthread_self()->获取当前线程的标识符
     */
    pthread_setname_np(pthread_self(), thread->m_name.substr(0, 15).c_str());

    std::function<void()> cb;
    cb.swap(thread->m_cb);

    thread->m_semaphore.notify();

    cb();
    return 0;
}

}
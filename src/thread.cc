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

Thread::Thread(std::function<void()> cb, const std::string &name) {
    if(name.empty()){
        m_name = "UNKNOW";
    }
    int rt = pthread_create(&m_thread, nullptr, &Thread::run, this);
    if(rt){
        DHT_LOG_ERROR(g_logger) << "pthread_create thread failed, rt=" << rt
            << " name=" << name;
        throw std::logic_error("pthread_create error");
    }
}
Thread::~Thread() {
    if(m_thread){
        pthread_detach(m_thread);
    }
}

void Thread::join() {
    if(m_thread){
        int rt = pthread_join(m_thread, nullptr);
        if(rt){
            HT_LOG_ERROR(g_logger) << "pthread_join thread failed, rt=" << rt
                                   << " name=" << name;
            throw std::logic_error("pthread_join error");
        }
        m_thread = 0;
    }
}

void *Thread::run(void *arg) {
    Thread* thread = (Thread*)arg;
    t_thread = thread;
    thread->m_id = dht::GetThreadId();
    pthread_setname_np(pthread_self(), thread->m_name.substr(0,15).c_str());

    std::function<void()> cb;
    cb.swap(thread->m_cb);

    cb();
    return 0;
}

}
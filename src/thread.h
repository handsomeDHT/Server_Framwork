//
// 线程
//

#ifndef SERVER_FRAMWORK_THREAD_H
#define SERVER_FRAMWORK_THREAD_H

#include <thread>
#include <functional>
#include <memory>
#include <pthread.h>
#include <semaphore.h>
#include <stdint.h>
#include <atomic>
#include "noncopyable.h"
#include "mutex.h"

namespace dht{

//线程
class Thread : Noncopyable{
public:
    typedef std::shared_ptr<Thread> ptr;
    //构造函数，[cb]线程执行函数，[name]线程名称
    Thread(std::function<void()> cb, const std::string& name);
    ~Thread();

    pid_t getId() const { return m_id; }
    const std::string& getName() const { return m_name; }

    //等待线程执行完成
    void join();

    static Thread* GetThis();  //拿到自己当前的线程
    static const std::string& GetName();
    static void SetName(const std::string& name);
private:
    Thread(const Thread&) = delete;
    Thread(const Thread&&) = delete;
    Thread& operator=(const Thread&) = delete;

    static void* run(void* arg);
private:
    pid_t m_id = -1;
    pthread_t m_thread = 0;
    std::function<void()> m_cb;
    std::string m_name;
    //信号量
    Semaphore m_semaphore;
};
}



#endif //SERVER_FRAMWORK_THREAD_H

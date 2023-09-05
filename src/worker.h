//
// Created by 帅帅的涛 on 2023/9/5.
//

#ifndef SERVER_FRAMWORK_WORKER_H
#define SERVER_FRAMWORK_WORKER_H

#include "mutex.h"
#include "singleton.h"
#include "log.h"
#include "iomanager.h"

namespace dht {

class WorkerGroup : Noncopyable, public std::enable_shared_from_this<WorkerGroup> {
public:
    typedef std::shared_ptr<WorkerGroup> ptr;
    static WorkerGroup::ptr Create(uint32_t batch_size, dht::Scheduler* s = dht::Scheduler::GetThis()) {
        return std::make_shared<WorkerGroup>(batch_size, s);
    }

    WorkerGroup(uint32_t batch_size, dht::Scheduler* s = dht::Scheduler::GetThis());
    ~WorkerGroup();

    void schedule(std::function<void()> cb, int thread = -1);
    void waitAll();
private:
    void doWork(std::function<void()> cb);
private:
    uint32_t m_batchSize;
    bool m_finish;
    Scheduler* m_scheduler;
    FiberSemaphore m_sem;
};

class WorkerManager {
public:
    WorkerManager();
    void add(Scheduler::ptr s);
    Scheduler::ptr get(const std::string& name);
    IOManager::ptr getAsIOManager(const std::string& name);

    template<class FiberOrCb>
    void schedule(const std::string& name, FiberOrCb fc, int thread = -1) {
        auto s = get(name);
        if(s) {
            s->schedule(fc, thread);
        } else {
            static dht::Logger::ptr s_logger = DHT_LOG_NAME("system");
            DHT_LOG_ERROR(s_logger) << "schedule name=" << name
                                      << " not exists";
        }
    }

    template<class Iter>
    void schedule(const std::string& name, Iter begin, Iter end) {
        auto s = get(name);
        if(s) {
            s->schedule(begin, end);
        } else {
            static dht::Logger::ptr s_logger = DHT_LOG_NAME("system");
            DHT_LOG_ERROR(s_logger) << "schedule name=" << name
                                      << " not exists";
        }
    }

    bool init();
    bool init(const std::map<std::string, std::map<std::string, std::string> >& v);
    void stop();

    bool isStoped() const { return m_stop;}
    std::ostream& dump(std::ostream& os);

    uint32_t getCount();
private:
    std::map<std::string, std::vector<Scheduler::ptr> > m_datas;
    bool m_stop;
};

typedef dht::Singleton<WorkerManager> WorkerMgr;

}



#endif //SERVER_FRAMWORK_WORKER_H

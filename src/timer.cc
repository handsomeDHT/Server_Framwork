//
// Created by 帅帅的涛 on 2023/7/17.
//

#include "timer.h"
#include "util.h"

namespace dht {
Timer::Timer(uint64_t ms, std::function<void()> cb
             , bool recurring, TimerManager *manager)
      :m_recurring(recurring)
      ,m_ms(ms)
      ,m_cb(cb)
      ,m_manager(manager){
    m_next = dht::GetCurrentMS() + m_ms;

}

Timer::Timer(uint64_t next)
        :m_next(next){
}

bool Timer::cancel() {
    TimerManager::RWMutexType::WriteLock lock(m_manager->m_mutex);
    if(m_cb) {
        m_cb = nullptr;
        //尝试在定时器集合中找到该计时器
        auto it = m_manager->m_timers.find(shared_from_this());
        m_manager->m_timers.erase(it);
        return true;
    }
    return false;
}

//尝试刷新计时器的执行时间并更新计时器管理器中的计时器列表。
bool Timer::refresh() {
    TimerManager::RWMutexType::WriteLock lock(m_manager->m_mutex);
    if(!m_cb) {
        return false;
    }
    auto it = m_manager->m_timers.find(shared_from_this());
    // 如果在计时器管理器的计时器列表中未找到当前计时器，则刷新失败。
    if(it == m_manager->m_timers.end()) {
        return false;
    }
    m_manager->m_timers.erase(it);
    // 更新计时器的下一次执行时间为当前时间加上指定的时间间隔（m_ms）。
    m_next = dht::GetCurrentMS() + m_ms;
    m_manager->m_timers.insert(shared_from_this());
    return true;
}

bool Timer::reset(uint64_t ms, bool from_now) {
    if(ms == m_ms && !from_now) {
        return true;
    }
    TimerManager::RWMutexType::WriteLock lock(m_manager->m_mutex);
    if(!m_cb) {
        return false;
    }
    auto it = m_manager->m_timers.find(shared_from_this());
    if(it == m_manager->m_timers.end()) {
        return false;
    }
    m_manager->m_timers.erase(it);
    // 计算新的计时器的起始时间。
    uint64_t start = 0;
    if(from_now) {
        start = dht::GetCurrentMS(); // 从当前时间开始计时。
    } else {
        start = m_next - m_ms;// 保持原有下一次执行时间不变。
    }

    // 更新计时器的时间间隔（ms）和下一次执行时间。
    m_ms = ms;
    m_next = start + m_ms;
    // 将更新后的计时器重新插入到计时器管理器的计时器列表中，确保位置正确。
    m_manager->addTimer(shared_from_this(), lock);
    return true;
}

bool Timer::Comparator::operator()(const Timer::ptr &lhs, const Timer::ptr &rhs) const {
    if(!lhs && !rhs){
        return false;
    }
    if(!lhs){
        return true;
    }
    if(!rhs){
        return false;
    }
    if(lhs->m_next < rhs->m_next){
        return true;
    }
    if(rhs->m_next < lhs->m_next){
        return false;
    }
    return lhs.get() < rhs.get();
}

TimerManager::TimerManager() {
    m_previouseTime = dht::GetCurrentMS();
}

TimerManager::~TimerManager() {

}

Timer::ptr TimerManager::addTimer(uint64_t ms
                                  , std::function<void()> cb
                                  , bool recurring) {
    Timer::ptr timer(new Timer(ms, cb, recurring, this));
    RWMutexType::WriteLock lock(m_mutex);
    addTimer(timer, lock);
    return timer;
}

static void OnTimer(std::weak_ptr<void> weak_cond, std::function<void()> cb) {
    std::shared_ptr<void> tmp = weak_cond.lock();
    if(tmp) {
        cb();
    }
}

Timer::ptr TimerManager::addConditionTimer(uint64_t ms, std::function<void()> cb
                                        , std::weak_ptr<void> weak_cond
                                        , bool recurring) {
    return addTimer(ms, std::bind(&OnTimer, weak_cond, cb), recurring);
}

uint64_t TimerManager::getNextTimer() {
    RWMutexType::ReadLock lock(m_mutex);
    m_tickled = false;
    if(m_timers.empty()){
        return ~0ull;
    }

    const Timer::ptr& next = *m_timers.begin();
    uint64_t now_ms = dht::GetCurrentMS();
    if(now_ms >= next->m_next){
        return 0;
    } else {
        return next->m_next - now_ms;
    }
}

void TimerManager::listExpiredCb(std::vector<std::function<void()>> &cbs) {
    uint64_t now_ms = dht::GetCurrentMS();
    //存放已经超时的数组
    std::vector<Timer::ptr> expired;

    {
        RWMutexType::ReadLock lock(m_mutex);
        if(m_timers.empty()){
            return;
        }
    }
    RWMutexType::WriteLock lock(m_mutex);
    if(m_timers.empty()) {
        return;
    }
    //检查服务器的时间是否调后了
    bool rollover = detectClockRollover(now_ms);
    if(!rollover && (*m_timers.begin())->m_next > now_ms){
        return;
    }

    Timer::ptr now_timer(new Timer(now_ms));
    auto it = rollover ? m_timers.end() : m_timers.lower_bound(now_timer);
    // 跳过下一次执行时间与当前时间相等的计时器，因为它们仍在执行中。
    while(it != m_timers.end() && (*it)->m_next == now_ms){
        ++it;
    }
    // 将超时的计时器添加到已超时数组中。
    expired.insert(expired.begin(), m_timers.begin(), it);
    // 从计时器列表中移除已超时的计时器。
    m_timers.erase(m_timers.begin(),it);
    cbs.reserve(expired.size());

    for(auto& timer : expired){
        cbs.push_back(timer->m_cb);
        if(timer->m_recurring){
            timer->m_next = now_ms + timer->m_ms;
            m_timers.insert(timer);
        } else {
            timer->m_cb = nullptr;
        }
    }
}

void TimerManager::addTimer(Timer::ptr val, RWMutexType::WriteLock &lock) {
    auto it = m_timers.insert(val).first;
    bool at_front = (it == m_timers.begin()) && !m_tickled;
    if(at_front){
        m_tickled = true;
    }
    lock.unlock();

    if(at_front) {
        onTimerInsertedAtFront();
    }
}

bool TimerManager::detectClockRollover(uint64_t now_ms) {
    bool rollover = false;
    if(now_ms < m_previouseTime &&
       now_ms < (m_previouseTime - 60 * 60 * 1000)) {
        rollover = true;
    }
    m_previouseTime = now_ms;
    return rollover;
}

bool TimerManager::hasTimer() {
    RWMutexType::ReadLock lock(m_mutex);
    return !m_timers.empty();
}

} // dht
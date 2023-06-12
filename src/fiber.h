//
// 协程
//

#ifndef SERVER_FRAMWORK_FIBER_H
#define SERVER_FRAMWORK_FIBER_H

#include <memory>
#include <ucontext.h>
#include <functional>
#include "dht.h"

namespace dht{
class Fiber : public std::enable_shared_from_this<Fiber>{
public:
    typedef std::shared_ptr<Fiber> ptr;

    enum State {
        INIT,
        HOLD,
        EXEC,
        TERM,
        READY
    };
private:
    Fiber();

public:
    Fiber(std::function<void()> cb, size_t stacksize = 0, bool use_caller = false);
    ~Fiber();

    void reset(std::function<void()> cb);
    //切换到当前协程执行
    void swapIn();
    //切换到后台执行
    void swapOut();

public:
    //设置当前协程
    static void SetThis(Fiber* f);
    //返回当前协程
    static Fiber::ptr GetThis();
    //协程切换到后台并设置Ready、Hold状态
    static void YieldToReady();
    static void YieldToHold();
    //总协程数
    static uint64_t TotalFibers();

    static void MainFunc();
    static void CallerMainFunc();

private:
    uint64_t m_id = 0;
    uint32_t m_stacksize = 0;
    State m_state = INIT;

    ucontext_t m_ctx;
    void* m_stack = nullptr;

    std::function<void()> m_cb;
};

}


#endif //SERVER_FRAMWORK_FIBER_H

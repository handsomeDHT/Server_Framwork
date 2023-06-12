//
// Created by 帅帅的涛 on 2023/6/11.
//

#include "fiber.h"
#include <atomic>

namespace dht{

static Logger::ptr g_logger = DHT_LOG_NAME("system");

static std::atomic<uint64_t> s_fiber_id {0};
static std::atomic<uint64_t> s_fiber_count {0};

static thread_local Fiber* t_fiber = nullptr;
static thread_local std::shared_ptr<Fiber::ptr> t_threadFiber =nullptr;

static ConfigVar<uint32_t>::ptr g_fiber_stack_size =
        Config::Lookup<uint32_t>("fiber.stack_size", 128 * 1024, "fiber stack size");

class MallocStackAllocator {
public:
    static void* Alloc(size_t size){
        return malloc(size);
    }

    static void Dealloc(void* vp, std::size_t) {
        return free(vp);
    }
};

using StackAllocator = MallocStackAllocator;

Fiber::Fiber() {
    m_state = EXEC;
    SetThis(this);

    if(getcontext(&m_ctx)){
        DHT_ASSERT2(false, "getcontext");
    }

    ++s_fiber_count;
    DHT_LOG_DEBUG(g_logger) << "Fiber::Fiber main";
}

Fiber::Fiber(std::function<void()> cb, size_t stacksize, bool use_caller)
    :m_id(++s_fiber_id)
    ,m_cb(cb) {
    ++s_fiber_count;
    m_stacksize = stacksize ? stacksize : g_fiber_stack_size->getValue();

    m_stack = StackAllocator::Alloc(m_stacksize);
    if(getcontext(&m_ctx)) {
        DHT_ASSERT2(false, "getcontext");
    }
    m_ctx.uc_link = nullptr;
    m_ctx.uc_stack.ss_sp = m_stack;
    m_ctx.uc_stack.ss_size = m_stacksize;

     if(!use_caller) {
         makecontext(&m_ctx, &Fiber::MainFunc, 0);
     } else {
         makecontext(&m_ctx, &Fiber::CallerMainFunc, 0);
     }

    DHT_LOG_DEBUG(g_logger) << "Fiber::Fiber id=" << m_id;
}

Fiber::~Fiber() {
    --s_fiber_count;
    if(m_stack){
        DHT_ASSERT(m_state == TERM || m_state == INIT);
        StackAllocator::Dealloc(m_stack, m_stacksize);
    }else{
        DHT_ASSERT(!m_cb);
        DHT_ASSERT(m_state == EXEC);

        Fiber* cur = t_fiber;
        if(cur == this) {
            SetThis(nullptr);
        }
    }
    DHT_LOG_DEBUG(g_logger) << "Fiber::~fiber id=" << m_id
                            << "total= " << s_fiber_count;
}

void Fiber::swapIn() {}

void Fiber::swapOut() {}

void Fiber::SetThis(Fiber *f) {}

Fiber::ptr Fiber::GetThis() {}

void Fiber::YieldToReady() {}

void Fiber::YieldToHold() {}

uint64_t Fiber::TotalFibers() {}

void Fiber::MainFunc() {}

}
//
// Created by 帅帅的涛 on 2023/6/13.
//

#include "src/dht.h"

dht::Logger::ptr g_logger = DHT_LOG_ROOT();

void run_in_fiber() {
    DHT_LOG_INFO(g_logger) << "run_in_fiber begin";
    dht::Fiber::YieldToHold();
    DHT_LOG_INFO(g_logger) << "run_in_fiber end";
    dht::Fiber::YieldToHold();
}

void test_fiber() {
    DHT_LOG_INFO(g_logger) << "main begin -1";
    {
        dht::Fiber::GetThis();
        DHT_LOG_INFO(g_logger) << "main begin";
        dht::Fiber::ptr fiber(new dht::Fiber(run_in_fiber));
        fiber->swapIn();
        DHT_LOG_INFO(g_logger) << "main after swapIn";
        fiber->swapIn();
        DHT_LOG_INFO(g_logger) << "main after end";
        fiber->swapIn();
    }
    DHT_LOG_INFO(g_logger) << "main after end2";
}

int main(int argc, char** argv){
    dht::Thread::SetName("test1");
    std::vector<dht::Thread::ptr> thrs;
    for(int i = 0; i < 3; ++i) {
        thrs.push_back(dht::Thread::ptr(
                new dht::Thread(&test_fiber, "name_" + std::to_string(i))));
    }
    for(auto i : thrs){
        i -> join();
    }

    return 0;
}
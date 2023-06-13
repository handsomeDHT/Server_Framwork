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

int main(int argc, char** argv){
    dht::Fiber::GetThis();
    DHT_LOG_INFO(g_logger) << "main begin";
    dht::Fiber::ptr fiber(new dht::Fiber(run_in_fiber));
    fiber->swapIn();
    DHT_LOG_INFO(g_logger) << "main after swapIn";
    fiber->swapIn();
    DHT_LOG_INFO(g_logger) << "main after end";
    return 0;
}
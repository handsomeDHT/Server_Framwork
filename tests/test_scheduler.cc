//
// Created by 帅帅的涛 on 2023/6/25.
//
#include "src/dht.h"

static dht::Logger::ptr g_logger = DHT_LOG_ROOT();

void test_fiber() {
    static int s_count = 5;
    DHT_LOG_INFO(g_logger) << "test in fiber s_count=" << s_count;

    sleep(1);
    if(--s_count >= 0) {
        //dht::Scheduler::GetThis()->schedule(&test_fiber, dht::GetThreadId());
        dht::Scheduler::GetThis()->schedule(&test_fiber);
    }
}

int main(int argc, char **argv){
    DHT_LOG_INFO(g_logger) << "main";
    dht::Scheduler sc(3, false, "test");
    sc.start();
    sleep(2);
    DHT_LOG_INFO(g_logger) << "schedule";
    sc.schedule(&test_fiber);
    sc.stop();
    DHT_LOG_INFO(g_logger) << "over";
    return 0;
}
//
// Created by 帅帅的涛 on 2023/5/19.
//

#include "src/dht.h"

dht::Logger::ptr g_logger = DHT_LOG_ROOT();

void fun1(){
    DHT_LOG_INFO(g_logger) << "name: " << dht::Thread::GetName()
                            << " this.name: " << dht::Thread::GetThis()->getName()
                            << " id: " << dht::GetThreadId()
                            << " this.id: " << dht::Thread::GetThis()->getId();
}

void fun2(){
}

int main(int argc, char** argv){
    DHT_LOG_INFO(g_logger) << "Thread test begin";
    //线程池
    std::vector<dht::Thread::ptr> thrs;
    for(int i = 0; i < 5; ++i){
        dht::Thread::ptr thr(new dht::Thread(&fun1, "name" + std::to_string(i)));
        thrs.push_back(thr);
    }
    for(int i = 0; i < 5; ++i){
        thrs[i] ->join();
    }
    DHT_LOG_INFO(g_logger) << "Thread test end";
    return 0;
}

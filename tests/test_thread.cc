//
// Created by 帅帅的涛 on 2023/5/19.
//

#include "src/dht.h"

dht::Logger::ptr g_logger = DHT_LOG_ROOT();

int count = 0;
dht::Mutex s_mutex;

void fun1(){
    DHT_LOG_INFO(g_logger) << "name: " << dht::Thread::GetName()
                           << " this.name: " << dht::Thread::GetThis()->getName()
                           << " id: " << dht::GetThreadId()
                           << " this.id: " << dht::Thread::GetThis()->getId();
    for(int i =0; i < 100; i++){
        //dht::RWMutex::WriteLock lock(s_mutex);
        dht::Mutex::Lock lock(s_mutex);
        count++;
    }
}

void fun2() {
    while(true) {
        DHT_LOG_INFO(g_logger) << "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";
    }
}

void fun3() {
    while(true) {
        DHT_LOG_INFO(g_logger) << "========================================";
    }
}

int main(int argc, char** argv){
    DHT_LOG_INFO(g_logger) << "thread test begin";
    YAML::Node root = YAML::LoadFile("/home/Server_Framwork/bin/conf/log2.yml");
    dht::Config::LoadFromYaml(root);

    std::vector<dht::Thread::ptr> thrs;

    for(int i = 0; i < 1; ++i) {
        dht::Thread::ptr thr(new dht::Thread(&fun2, "name_" + std::to_string(i * 2)));
        dht::Thread::ptr thr2(new dht::Thread(&fun3, "name_" + std::to_string(i * 2 + 1)));
        thrs.push_back(thr);
        thrs.push_back(thr2);
    }

    for(size_t i = 0; i < thrs.size(); ++i) {
        thrs[i]->join();
    }
    DHT_LOG_INFO(g_logger) << "thread test end";
    DHT_LOG_INFO(g_logger) << "count=" << count;

    return 0;
}
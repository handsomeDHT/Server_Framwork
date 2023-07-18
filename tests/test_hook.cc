//
// Created by 帅帅的涛 on 2023/7/18.
//
#include "src/dht.h"

dht::Logger::ptr g_logger = DHT_LOG_ROOT();

void test_sleep(){
    dht::IOManager iom(1);

    iom.schedule([](){
        sleep(2);
        DHT_LOG_INFO(g_logger) << "sleep 2";
    });

    iom.schedule([](){
        sleep(3);
        DHT_LOG_INFO(g_logger) << "sleep 3";
    });
    DHT_LOG_INFO(g_logger) << "test sleep";
}

int main(int argc, char** argv){
    test_sleep();
    return 0;
}

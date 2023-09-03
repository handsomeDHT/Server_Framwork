//
// Created by 帅帅的涛 on 2023/9/3.
//
#include "src/daemon.h"
#include "src/iomanager.h"
#include "src/log.h"

static dht::Logger::ptr g_logger = DHT_LOG_ROOT();

dht::Timer::ptr timer;
int server_main(int argc, char** argv) {
    DHT_LOG_INFO(g_logger) << dht::ProcessInfoMgr::GetInstance()->toString();
    dht::IOManager iom(1);
    timer = iom.addTimer(1000, [](){
        DHT_LOG_INFO(g_logger) << "onTimer";
        static int count = 0;
        if(++count > 10) {
            exit(1);
        }
    }, true);
    return 0;
}

int main(int argc, char** argv) {
    return dht::start_daemon(argc, argv, server_main, argc != 1);
}
//
// Created by 帅帅的涛 on 2023/6/25.
//
#include "src/dht.h"

static dht::Logger::ptr g_logger = DHT_LOG_ROOT();

int main(int argc, char **argv){
    dht::Scheduler sc;
    sc.start();
    sc.stop();
}
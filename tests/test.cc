//
// Created by 帅帅的涛 on 2023/3/28.
//
#include <iostream>
#include "../src/log.h"
#include "../src/util.h"

int main(int argc, char** argv){
    dht::Logger::ptr logger(new dht::Logger);
    logger->addAppender(dht::LogAppender::ptr(new dht::StdoutLogAppender));

    //dht::LogEvent::ptr event(new dht::LogEvent(__FILE__, __LINE__, 0, dht::GetThreadId(), dht::GetFiberId(), time(0)));
    //logger->log(dht::LogLevel::DEBUG, event);

    DHT_LOG_INFO(logger) << "test macro";
    DHT_LOG_ERROR(logger) << "test macro error";
    return 0;
}

//
// Created by 帅帅的涛 on 2023/3/28.
//
#include <iostream>
#include "../src/log.h"
#include "../src/util.h"

int main(int argc, char** argv){
    dht::Logger::ptr logger(new dht::Logger);
    logger->addAppender(dht::LogAppender::ptr(new dht::StdoutLogAppender));

    dht::FileLogAppender::ptr file_appender(new dht::FileLogAppender("./log.txt"));

    dht::LogFormatter::ptr fmt(new dht::LogFormatter("%d%T%p%T%m%n"));
    file_appender->setFormatter(fmt);
    file_appender->setLevel(dht::LogLevel::ERROR);

    logger->addAppender(file_appender);

    //dht::LogEvent::ptr event(new dht::LogEvent(__FILE__, __LINE__, 0, dht::GetThreadId(), dht::GetFiberId(), time(0)));
    //logger->log(dht::LogLevel::DEBUG, event);

    DHT_LOG_INFO(logger) << "test macro";
    DHT_LOG_ERROR(logger) << "test macro error";

    DHT_LOG_FMT_ERROR(logger, "test macro fmt error %s", "aa");
    auto l = dht::LoggerMgr::GetInstance()->getLogger("xx");
    DHT_LOG_INFO(l) << "xxx";

    return 0;
}

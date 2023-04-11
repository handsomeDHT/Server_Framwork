//
// Created by 帅帅的涛 on 2023/3/28.
//
#include <iostream>
#include "../src/log.h"
#include "../src/util.h"

int main(int argc, char** argv){
    //创建一个日志事件logger
    dht::Logger::ptr logger(new dht::Logger);

    //logger增加控制台输出器。
    logger->addAppender(dht::LogAppender::ptr(new dht::StdoutLogAppender));

    //增加文件输出器
    dht::LogFormatter::ptr fmt(new dht::LogFormatter("%d%T%p%T%m%n"));
    dht::FileLogAppender::ptr file_appender(new dht::FileLogAppender("./log.txt"));
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

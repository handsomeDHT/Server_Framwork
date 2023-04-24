#ifndef SERVER_FRAMWORK_LOG_H
#define SERVER_FRAMWORK_LOG_H

#include <string>
#include <cstdint>
#include <memory>
#include <list>
#include <sstream>
#include <fstream>
#include <vector>
#include <iostream>
#include <map>
#include <functional>
#include <cstdarg>
#include <stdarg.h>
#include <map>
#include "singleton.h"



/**
 * @brief 使用流式方式将日志级别level的日志写入到logger
 */
#define DHT_LOG_LEVEL(logger, level) \
    if(logger->getLevel() <= level) \
        dht::LogEventWrap(dht::LogEvent::ptr(new dht::LogEvent(logger, level, \
                        __FILE__, __LINE__, 0, dht::GetThreadId(),\
                dht::GetFiberId(), time(0)/*, dht::Thread::GetName()*/))).getSS()


/**
 * @brief 使用流式方式将日志级别debug的日志写入到logger
 */
#define DHT_LOG_DEBUG(logger) DHT_LOG_LEVEL(logger, dht::LogLevel::DEBUG)

/**
 * @brief 使用流式方式将日志级别info的日志写入到logger
 */
#define DHT_LOG_INFO(logger) DHT_LOG_LEVEL(logger, dht::LogLevel::INFO)

/**
 * @brief 使用流式方式将日志级别warn的日志写入到logger
 */
#define DHT_LOG_WARN(logger) DHT_LOG_LEVEL(logger, dht::LogLevel::WARN)

/**
 * @brief 使用流式方式将日志级别error的日志写入到logger
 */
#define DHT_LOG_ERROR(logger) DHT_LOG_LEVEL(logger, dht::LogLevel::ERROR)

/**
 * @brief 使用流式方式将日志级别fatal的日志写入到logger
 */
#define DHT_LOG_FATAL(logger) DHT_LOG_LEVEL(logger, dht::LogLevel::FATAL)

/**
 * @brief 使用格式化方式将日志级别level的日志写入到logger
 */
#define DHT_LOG_FMT_LEVEL(logger, level, fmt, ...) \
    if(logger->getLevel() <= level)                \
        dht::LogEventWrap(dht::LogEvent::ptr(new dht::LogEvent(logger, level, \
                        __FILE__, __LINE__, 0, dht::GetThreadId(),\
                dht::GetFiberId(), time(0)/*, dht::Thread::GetName()*/))).getEvent()->format(fmt, __VA_ARGS__)

/**
 * @brief 使用格式化方式将日志级别debug的日志写入到logger
 */
#define DHT_LOG_FMT_DEBUG(logger, fmt, ...) DHT_LOG_FMT_LEVEL(logger, dht::LogLevel::DEBUG, fmt, __VA_ARGS__)

/**
 * @brief 使用格式化方式将日志级别info的日志写入到logger
 */
#define DHT_LOG_FMT_INFO(logger, fmt, ...)  DHT_LOG_FMT_LEVEL(logger, dht::LogLevel::INFO, fmt, __VA_ARGS__)

/**
 * @brief 使用格式化方式将日志级别warn的日志写入到logger
 */
#define DHT_LOG_FMT_WARN(logger, fmt, ...)  DHT_LOG_FMT_LEVEL(logger, dht::LogLevel::WARN, fmt, __VA_ARGS__)

/**
 * @brief 使用格式化方式将日志级别error的日志写入到logger
 */
#define DHT_LOG_FMT_ERROR(logger, fmt, ...) DHT_LOG_FMT_LEVEL(logger, dht::LogLevel::ERROR, fmt, __VA_ARGS__)

/**
 * @brief 使用格式化方式将日志级别fatal的日志写入到logger
 */
#define DHT_LOG_FMT_FATAL(logger, fmt, ...) DHT_LOG_FMT_LEVEL(logger, dht::LogLevel::FATAL, fmt, __VA_ARGS__)

/**
 * @brief 获取主日志器
 */
#define DHT_LOG_ROOT() dht::LoggerMgr::GetInstance()->getRoot()

/**
 * @brief 获取name的日志器
 */
#define DHT_LOG_NAME(name) dht::LoggerMgr::GetInstance()->getLogger(name)


namespace dht{
class Logger;
class LoggerManager;
/**
 * @brief 日志级别
 */
class LogLevel {
public:
    enum Level {
        UNKNOW = 0,
        DEBUG = 1,
        INFO = 2,
        WARN = 3,
        ERROR = 4,
        FATAL = 5
    };
    static const char* ToString(LogLevel::Level level);
};

/**
 * @brief 日志事件
 */
class LogEvent {
public:
    typedef std::shared_ptr<LogEvent> ptr;
    LogEvent(std::shared_ptr<Logger> logger
             ,LogLevel::Level level
             ,const char* file
             ,int32_t m_line, uint32_t elapse
             ,uint32_t thread_id
             ,uint32_t fiber_id
             ,uint64_t time);

    const char* getFile() const { return m_file; }
    int32_t getLine() const { return m_line;}
    uint32_t getElapse() const {return m_elapse;}
    uint32_t getThreadId() const {return m_threadId;}
    uint32_t getFiberID() const {return m_fiberId;}
    uint64_t getTime() const {return m_time;}
    std::stringstream& getSS() {return m_ss;}
    std::string getContent() const { return m_ss.str();}
    const std::string& getThreadName() const { return m_threadName;}
    std::shared_ptr<Logger> getLogger() const {return m_logger; }
    LogLevel::Level getLevel() const { return m_level;}

    void format(const char* fmt, ...);
    void format(const char* fmt, va_list al);

private:
    const char *m_file = nullptr;  //文件地址
    int32_t m_line = 0;              //行号
    uint32_t m_elapse = 0;           //程序启动到现在的毫秒数
    uint32_t m_threadId = 0;       //线程ID
    uint32_t m_fiberId = 0;        //协程ID
    uint64_t m_time = 0;           //时间戳
    std::stringstream m_ss;
    std::string m_content;         //消息
    std::string m_threadName;      //线程名称
    std::shared_ptr<Logger> m_logger;
    LogLevel::Level m_level;

};

/**
 * @brief 日志事件包装器
 */
class LogEventWrap{
public:
    LogEventWrap(LogEvent::ptr e);
    ~LogEventWrap();

    LogEvent::ptr getEvent() const {return m_event; }

    std::stringstream& getSS();
private:
    LogEvent::ptr m_event;
};

/**
 * @brief 日志格式器
     * @brief 构造函数
     * @param[in] pattern 格式模板
     * @details
     *  %m 消息
     *  %p 日志级别
     *  %r 累计毫秒数
     *  %c 日志名称
     *  %t 线程id
     *  %n 换行
     *  %d 时间
     *  %f 文件名
     *  %l 行号
     *  %T 制表符
     *  %F 协程id
     *  %N 线程名称
     *  默认格式 "%d{%Y-%m-%d %H:%M:%S}%T%t%T%N%T%F%T[%p]%T[%c]%T%f:%l%T%m%n"
 */
class LogFormatter {
public:
    typedef std::shared_ptr<LogFormatter> ptr;
    explicit LogFormatter(const std::string& pattern);
    //将LogEvent格式化成字符串
    std::string format(std::shared_ptr<Logger> logger
                       ,LogLevel::Level level
                       ,LogEvent::ptr event);

public:
    /**
     * 日志内容项格式化
     */
    class FormatItem {
    public:
        typedef std::shared_ptr<FormatItem> ptr;
        virtual ~FormatItem() {}
        //将对应的日志格式内容写到os中
        virtual void format(std::ostream &os
                            , std::shared_ptr<Logger> logger
                            , LogLevel::Level level
                            , LogEvent::ptr event) = 0;
    };
    void init();

    bool isError() const { return m_error;}

private:
    //日志格式
    std::string m_pattern;
    //通过日志格式解析出来的FormatItem
    std::vector<FormatItem::ptr> m_items;
    bool m_error = false;
};

/**
 * @brief 日志输出地
 */
class LogAppender {
public:
    typedef std::shared_ptr<LogAppender> ptr;
    virtual ~LogAppender() {};
    /**
    * @brief 写入日志
    * @param[in] logger 日志器
    * @param[in] level 日志级别
    * @param[in] event 日志事件
    */
    virtual void log(std::shared_ptr<Logger> logger
                     ,LogLevel::Level level
                     ,LogEvent::ptr event) = 0;

    void setFormatter(LogFormatter::ptr val) { m_formatter = val; }
    LogFormatter::ptr getFormatter() const { return m_formatter; }

    void setLevel(LogLevel::Level val) {m_level = val; }
    LogLevel::Level getLevel() const { return m_level; }

protected:
    LogLevel::Level m_level = LogLevel::DEBUG;
    LogFormatter::ptr m_formatter;
};

/**
 * @brief 日志器
 */
class Logger : public std::enable_shared_from_this<Logger> {
    friend class LoggerManager;

public:
    typedef std::shared_ptr<Logger> ptr;

    explicit Logger(const std::string& name = "root");
    void log(LogLevel::Level level, LogEvent::ptr event);

    void debug(LogEvent::ptr event);
    void info(LogEvent::ptr event);
    void warn(LogEvent::ptr event);
    void error(LogEvent::ptr event);
    void fatal(LogEvent::ptr event);
    void addAppender(LogAppender::ptr appender);
    void delAppender(LogAppender::ptr appender);
    LogLevel::Level getLevel() const { return m_level; }
    void setLevel(LogLevel::Level val) { m_level = val; }
    const std::string& getName() const{ return m_name; }

    void setFormatter(LogFormatter::ptr val);
    void clearAppenders();
private:
    std::string m_name;                                //日志名称
    LogLevel::Level m_level;                           //日志级别
    std::list<LogAppender::ptr> m_appenders;           //Appender集合
    LogFormatter::ptr m_formatter;                     //
    Logger::ptr m_root;
};

//输出到控制台的Appender
class StdoutLogAppender : public LogAppender {
public:
    typedef std::shared_ptr<StdoutLogAppender> ptr;

    virtual void log(Logger::ptr logger
                     ,LogLevel::Level level
                     ,LogEvent::ptr event) override;

private:
};

//输出到文件的Appender
class FileLogAppender : public LogAppender {
public:
    typedef std::shared_ptr<FileLogAppender> ptr;

    FileLogAppender(const std::string &filename);

    void log(Logger::ptr logger
             ,LogLevel::Level level
             ,LogEvent::ptr event) override;

//重新打开文件，文件打开成功返回true
    bool reopen();

private:
    std::string m_filename;
    std::ofstream m_filestream;
};

//日志管理器
class LoggerManager{
public:
    LoggerManager();
    Logger::ptr getLogger(const std::string& name);

    void init();
    Logger::ptr getRoot() const {return m_root;}
private:
    std::map<std::string, Logger::ptr> m_loggers;
    Logger::ptr m_root;
};

typedef dht::Singleton<LoggerManager> LoggerMgr;
}

#endif //SERVER_FRAMWORK_LOG_H

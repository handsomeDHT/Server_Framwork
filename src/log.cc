//
// Created by 帅帅的涛 on 2023/3/22.
//
#include "log.h"
#include <iostream>
#include <map>

namespace sylar {

const char* LogLevel::ToString(LogLevel::Level level) {
    switch(level){
#define XX(name) \
    case LogLevel::name: \
        return #name; \
        break;

        XX(DEBUG);
        XX(INFO);
        XX(WARN);
        XX(ERROR);
        XX(FATAL);
#undef XX
        default:
            return "UNKNOW";
    }
    return "UNKNOW";
}

/**---------------------------------------**/
/**--------------获取日志信息--------------**/
/**---------------------------------------**/
/**
%m->消息体，
%p->输出优先级：DEBUG, INFO, WARN
%r->从应用启动到日志输出消耗的毫秒数
%c->输出所属的类目，所在类的全名
%t->输出该日志事件的线程名
%n->输出回车换行符
%d->输出日志时间点的日期或时间
%f->文件名
%l->输出日志事件发生的位置，包括类目名，发生的线程，以及在代码中的行数
*/

class MessageFormatItem : public LogFormatter::FormatItem{
public:
    explicit MessageFormatItem(const std::string& str = "") {}
    void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override{
        os << event->getContent();
    }
};

class LevelFormatItem : public LogFormatter::FormatItem{
public:
    explicit LevelFormatItem(const std::string& str = "") {}
    void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override{
        os << LogLevel::ToString(level);
    }
};

class ElapseFormatItem : public LogFormatter::FormatItem{
public:
    explicit ElapseFormatItem(const std::string& str = "") {}
    void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override{
        os << event->getElapse();
    }
};

class NameFormatItem : public LogFormatter::FormatItem{
public:
    explicit NameFormatItem(const std::string& str = "") {}
    void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override{
        os << logger->getName();
    }
};

class ThreadIdFormatItem : public LogFormatter::FormatItem{
public:
    explicit ThreadIdFormatItem(const std::string& str = "") {}
    void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override{
        os << event->getThreadId();
    }
};

class FiberIdFormatItem : public LogFormatter::FormatItem{
public:
    explicit FiberIdFormatItem(const std::string& str = "") {}
    void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override{
        os << event->getFiberID();
    }
};

class DateTimeIdFormatItem : public LogFormatter::FormatItem{
public:
    explicit DateTimeIdFormatItem(const std::string format = "%Y-%m-%d %H:%M:%S")
            :m_format(format){
        if(m_format.empty()) {
            m_format = "%Y-%m-%d %H:%M:%S";
        }
    }
    void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override{
        os << event->getTime();
    }
private:
    std::string m_format;
};

class FilenameFormatItem : public LogFormatter::FormatItem{
public:
    explicit FilenameFormatItem(const std::string& str = "") {}
    void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override{
        os << event->getFile();
    }
};

class LineFormatItem : public LogFormatter::FormatItem{
public:
    explicit LineFormatItem(const std::string& str = "") {}
    void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override{
        os << event->getLine();
    }
};

class NewLineFormatItem : public LogFormatter::FormatItem{
public:
    explicit NewLineFormatItem(const std::string& str = "") {}
    void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override{
        os << std::endl;
    }
};

class StringFormatItem : public LogFormatter::FormatItem{
public:
    explicit StringFormatItem(const std::string& str)
        :m_string(str){}
    void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override{
        os << m_string;
    }
private:
    std::string m_string;
};

class TabFormatItem : public LogFormatter::FormatItem {
public:
    explicit TabFormatItem(const std::string& str = "") {}
    void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << "\t";
    }
private:
    std::string m_string;
};

class ThreadNameFormatItem : public LogFormatter::FormatItem {
public:
    explicit ThreadNameFormatItem(const std::string& str = "") {}
    void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << event->getThreadName();
    }
};



Logger::Logger(const std::string &name) : m_name(name) {
}

void Logger::addAppender(LogAppender::ptr appender) {
    m_appenders.push_back(appender);
}

void Logger::delAppender(LogAppender::ptr appender) {
    for (auto it = m_appenders.begin(); it != m_appenders.end(); it++) {
        if (*it == appender) {
            m_appenders.erase(it);
            break;
        }
    }
}

void Logger::log(LogLevel::Level level, LogEvent::ptr event) {
    if (level >= m_level) {
        for (auto &i: m_appenders) {
            i->log(level, event);
        }
    }
}

void Logger::debug(LogEvent::ptr event) {
    log(LogLevel::DEBUG, event);
}

void Logger::info(LogEvent::ptr event) {
    log(LogLevel::INFO, event);
}

void Logger::warn(LogEvent::ptr event) {
    log(LogLevel::WARN, event);
}

void Logger::error(LogEvent::ptr event) {
    log(LogLevel::ERROR, event);
}

void Logger::fatal(LogEvent::ptr event) {
    log(LogLevel::FATAL, event);
}

FileLogAppender::FileLogAppender(const std::string &filename) : m_filename(filename) {
}

void FileLogAppender::log(std::shared_ptr<Logger> logger , LogLevel::Level level, LogEvent::ptr event) {
    if (level >= m_level) {
        //m_filestream << m_formatter.format(event)原文写的是这样的
        m_filestream << m_formatter->format(logger, level, event);
    }
}

bool FileLogAppender::reopen() {
    if (m_filestream) {
        m_filestream.close();
    }
    m_filestream.open(m_filename);
    return !!m_filestream;
}

void StdoutLogAppender::log(std::shared_ptr<Logger> logger ,LogLevel::Level level, LogEvent::ptr event) {
    if (level >= m_level) {
        std::cout << m_formatter->format(logger, level ,event);
    }
}

LogFormatter::LogFormatter(const std::string& pattern)
        : m_pattern(pattern) {
}

//%xxx %xxx{xxx} %%
void LogFormatter::init() {
    //ptr, format, type
    std::vector<std::tuple<std::string , std::string , int>> vec;

    std::string nstr;
    //字符串m_pattern进行解析
    for(size_t i = 0; i < m_pattern.size(); ++i){
        if(m_pattern[i] != '%'){
            nstr.append(1,m_pattern[i]);
            continue;
        }

        if((i + 1) < m_pattern.size()){
            if(m_pattern[i + 1] == '%' ){
                nstr.append(1 , '%');
                continue;
            }
        }

        size_t n = i + 1;
        int fmt_status = 0;
        size_t fmt_begin = 0;

        std::string str;
        std::string fmt;
        while(++n < m_pattern.size()){
            if(std::isspace((m_pattern[n]))){
                break;
            }
            if(fmt_status == 0){
                if(m_pattern[n] == '{'){
                    str = m_pattern.substr(i + 1 , n - i - 1 );
                    fmt_status = 1; //解析格式
                    ++n;
                    fmt_begin = n;
                    continue;
                }
            }
            if(fmt_status == 1){
                if(m_pattern[n] == '}'){
                    fmt = m_pattern.substr(fmt_begin + 1 , n - fmt_begin - 1);
                    fmt_status = 2;
                    ++n;
                    break;
                }
            }
        }
        if(fmt_status == 0){
            if(!nstr.empty()){
                vec.emplace_back(nstr, "" ,0);
            }
            str = m_pattern.substr(i + 1, n - i - 1);
            vec.emplace_back(str , fmt , 1);
            i = n;
        }else if(fmt_status == 1){
            std::cout << "pattern parse error: " << m_pattern << " - " << m_pattern.substr(i) << std::endl;
            vec.emplace_back("<<pattern_error>>" , fmt , 1);
        }else if(fmt_status == 2){
            if(!nstr.empty()){
                vec.emplace_back(nstr, "" ,0);
            }
            vec.emplace_back(str , fmt , 1);
            i = n;
        }
    }
    if(!nstr.empty()){
        vec.emplace_back(nstr, "" ,0);
    }
    static std::map<std::string,
                    std::function<FormatItem::ptr(const std::string& str)>>
                    s_format_items ={
#define XX(str, C) \
            {#str,[](const std::string& fmt) {return FormatItem::ptr(new C(fmt));}}

            XX(m , MessageFormatItem),
            XX(p , LevelFormatItem),
            XX(r , ElapseFormatItem),
            XX(c , NameFormatItem),
            XX(t , ThreadIdFormatItem),
            XX(n , NewLineFormatItem),
            XX(d , DateTimeIdFormatItem),
            XX(f , FilenameFormatItem),
            XX(l , LineFormatItem),
            XX(T , TabFormatItem),
            XX(F , FiberIdFormatItem),
            XX(N , ThreadNameFormatItem),
#undef XX
    };

    for(auto& i : vec) {
        if(std::get<2>(i) == 0) {
            m_items.push_back(FormatItem::ptr(new StringFormatItem(std::get<0>(i))));
        } else {
            auto it = s_format_items.find(std::get<0>(i));
            if(it == s_format_items.end()) {
                m_items.push_back(FormatItem::ptr(new StringFormatItem("<<error_format %" + std::get<0>(i) + ">>")));
                m_error = true;
            } else {
                m_items.push_back(it->second(std::get<1>(i)));
            }
        }

        //std::cout << "(" << std::get<0>(i) << ") - (" << std::get<1>(i) << ") - (" << std::get<2>(i) << ")" << std::endl;
    }
    //std::cout << m_items.size() << std::endl;
}

std::string LogFormatter::format(std::shared_ptr<Logger> logger, LogLevel::Level level , LogEvent::ptr event) {
    std::stringstream ss;
    for (auto &i: m_items) {
        i->format(ss, logger, level, event);
    }
    return ss.str();
}

}
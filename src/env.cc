

#include "env.h"
#include "src/log.h"
#include <string.h>
#include <iostream>
#include <iomanip>
#include <unistd.h>
#include <stdlib.h>
#include "config.h"

namespace dht {

static dht::Logger::ptr g_logger = DHT_LOG_NAME("system");

bool Env::init(int argc, char** argv) {
    char link[1024] = {0}; // 用于存储符号链接的路径的字符数组
    char path[1024] = {0}; // 用于存储读取到的符号链接指向的可执行文件的路径的字符数组
    // 获取当前进程的可执行文件路径
    //将进程ID( getpid()获取 )以"/proc/%d/exe"的形式传送给link
    sprintf(link, "/proc/%d/exe", getpid());
    readlink(link, path, sizeof(path));
    // /path/xxx/exe
    m_exe = path;

    // 找到可执行文件路径中的最后一个斜杠("/")的位置
    auto pos = m_exe.find_last_of("/");
    // 从可执行文件路径中提取出当前工作目录并存储到成员变量 m_cwd 中
    m_cwd = m_exe.substr(0, pos) + "/";

    // 存储程序的名称（从命令行参数中获取的第一个参数，通常是可执行文件的名称）到成员变量 m_program 中
    m_program = argv[0];
    // -config /path/to/config -file xxxx -d
    // 解析命令行参数，格式为 "-key value"，并存储到成员变量中
    const char* now_key = nullptr;
    for(int i = 1; i < argc; ++i) {
        if(argv[i][0] == '-') {
            if(strlen(argv[i]) > 1) {
                if(now_key) {
                    // 将上一个键值对添加到成员变量中
                    add(now_key, "");
                }
                now_key = argv[i] + 1;
            } else {
                DHT_LOG_ERROR(g_logger) << "invalid arg idx=" << i
                                          << " val=" << argv[i];
                return false;
            }
        } else {
            if(now_key) {
                add(now_key, argv[i]);
                now_key = nullptr;
            } else {
                DHT_LOG_ERROR(g_logger) << "invalid arg idx=" << i
                                          << " val=" << argv[i];
                return false;
            }
        }
    }
    if(now_key) {
        add(now_key, "");
    }
    return true;
}

void Env::add(const std::string& key, const std::string& val) {
    RWMutexType::WriteLock lock(m_mutex);
    m_args[key] = val;
}

bool Env::has(const std::string& key) {
    RWMutexType::ReadLock lock(m_mutex);
    auto it = m_args.find(key);
    return it != m_args.end();
}

void Env::del(const std::string& key) {
    RWMutexType::WriteLock lock(m_mutex);
    m_args.erase(key);
}

std::string Env::get(const std::string& key, const std::string& default_value) {
    RWMutexType::ReadLock lock(m_mutex);
    auto it = m_args.find(key);
    return it != m_args.end() ? it->second : default_value;
}

void Env::addHelp(const std::string& key, const std::string& desc) {
    removeHelp(key);
    RWMutexType::WriteLock lock(m_mutex);
    m_helps.push_back(std::make_pair(key, desc));
}

void Env::removeHelp(const std::string& key) {
    RWMutexType::WriteLock lock(m_mutex);
    for(auto it = m_helps.begin();
        it != m_helps.end();) {
        if(it->first == key) {
            it = m_helps.erase(it);
        } else {
            ++it;
        }
    }
}

void Env::printHelp() {
    RWMutexType::ReadLock lock(m_mutex);
    std::cout << "Usage: " << m_program << " [options]" << std::endl;
    for(auto& i : m_helps) {
        std::cout << std::setw(5) << "-" << i.first << " : " << i.second << std::endl;
    }
}

bool Env::setEnv(const std::string& key, const std::string& val) {
    return !setenv(key.c_str(), val.c_str(), 1);
}

std::string Env::getEnv(const std::string& key, const std::string& default_value) {
    const char* v = getenv(key.c_str());
    if(v == nullptr) {
        return default_value;
    }
    return v;
}

std::string Env::getAbsolutePath(const std::string& path) const {
    if(path.empty()) {
        return "/";
    }
    if(path[0] == '/') {
        return path;
    }
    return m_cwd + path;
}

std::string Env::getAbsoluteWorkPath(const std::string& path) const {
    if(path.empty()) {
        return "/";
    }
    if(path[0] == '/') {
        return path;
    }
    static dht::ConfigVar<std::string>::ptr g_server_work_path =
            dht::Config::Lookup<std::string>("server.work_path");
    return g_server_work_path->getValue() + "/" + path;
}

std::string Env::getConfigPath() {
    return getAbsolutePath(get("c", "conf"));
}

}
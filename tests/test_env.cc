
#include "src/env.h"
#include <unistd.h>
#include <iostream>
#include <fstream>

//通过读取当前的线程，解析其中内容
//当前线程中运行的是文件test_env，那么解析出来就是这个
struct A {
    A() {
        // 打开当前进程的命令行参数文件
        std::ifstream ifs("/proc/" + std::to_string(getpid()) + "/cmdline", std::ios::binary);
        //std::cout << std::to_string(getpid());
        std::string content;
        // 预分配4096字节的空间来存储命令行参数
        content.resize(4096);

        // 从文件中读取命令行参数到 content 字符串
        ifs.read(&content[0], content.size());
        content.resize(ifs.gcount());// 调整字符串大小以匹配读取的实际字节数

        // 遍历 content 字符串并打印每个字符的索引、字符值和ASCII码值
        for(size_t i = 0; i < content.size(); ++i) {
            std::cout << i << " - " << content[i] << " - " << (int)content[i] << std::endl;
        }
    }
};

A a;

int main(int argc, char** argv) {

    std::cout << "argc=" << argc << std::endl;
    dht::EnvMgr::GetInstance()->addHelp("s", "start with the terminal");
    dht::EnvMgr::GetInstance()->addHelp("d", "run as daemon");
    dht::EnvMgr::GetInstance()->addHelp("p", "print help");
    if(!dht::EnvMgr::GetInstance()->init(argc, argv)) {
        dht::EnvMgr::GetInstance()->printHelp();
        return 0;
    }

    std::cout << "exe=" << dht::EnvMgr::GetInstance()->getExe() << std::endl;
    std::cout << "cwd=" << dht::EnvMgr::GetInstance()->getCwd() << std::endl;

    std::cout << "path=" << dht::EnvMgr::GetInstance()->getEnv("PATH", "xxx") << std::endl;
    std::cout << "test=" << dht::EnvMgr::GetInstance()->getEnv("TEST", "") << std::endl;
    std::cout << "set env " << dht::EnvMgr::GetInstance()->setEnv("TEST", "yy") << std::endl;
    std::cout << "test=" << dht::EnvMgr::GetInstance()->getEnv("TEST", "") << std::endl;
    if(dht::EnvMgr::GetInstance()->has("p")) {
        dht::EnvMgr::GetInstance()->printHelp();
    }
    return 0;
}
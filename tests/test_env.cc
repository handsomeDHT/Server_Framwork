//
// Created by 帅帅的涛 on 2023/9/3.
//
#include "src/env.h"
#include <unistd.h>
#include <iostream>
#include <fstream>

struct A {
    A() {
        std::ifstream ifs("/proc/" + std::to_string(getpid()) + "/cmdline", std::ios::binary);
        std::string content;
        content.resize(4096);

        ifs.read(&content[0], content.size());
        content.resize(ifs.gcount());

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
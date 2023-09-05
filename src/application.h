//
// Created by 帅帅的涛 on 2023/9/5.
//

#ifndef SERVER_FRAMWORK_APPLICATION_H
#define SERVER_FRAMWORK_APPLICATION_H


#include "src/http/http_server.h"

namespace dht {

class Application {
public:
    Application();

    static Application* GetInstance() { return s_instance;}
    bool init(int argc, char** argv);
    bool run();
private:
    int main(int argc, char** argv);
    int run_fiber();
private:
    int m_argc = 0;
    char** m_argv = nullptr;

    std::vector<dht::http::HttpServer::ptr> m_httpservers;
    IOManager::ptr m_mainIOManager;
    static Application* s_instance;
};

}


#endif //SERVER_FRAMWORK_APPLICATION_H

//
// Created by 帅帅的涛 on 2023/9/3.
//

#ifndef SERVER_FRAMWORK_ENV_H
#define SERVER_FRAMWORK_ENV_H


#include "src/singleton.h"
#include "src/thread.h"
#include <map>
#include <vector>

namespace dht {

class Env {
public:
    typedef RWMutex RWMutexType;
    bool init(int argc, char** argv);

    void add(const std::string& key, const std::string& val);
    bool has(const std::string& key);
    void del(const std::string& key);
    std::string get(const std::string& key, const std::string& default_value = "");

    void addHelp(const std::string& key, const std::string& desc);
    void removeHelp(const std::string& key);
    void printHelp();

    const std::string& getExe() const { return m_exe;}
    const std::string& getCwd() const { return m_cwd;}

    bool setEnv(const std::string& key, const std::string& val);
    std::string getEnv(const std::string& key, const std::string& default_value = "");
private:
    RWMutexType m_mutex;
    std::map<std::string, std::string> m_args;
    std::vector<std::pair<std::string, std::string> > m_helps;

    std::string m_program;
    std::string m_exe;
    std::string m_cwd;
};

typedef dht::Singleton<Env> EnvMgr;

}


#endif //SERVER_FRAMWORK_ENV_H

//
// Created by 帅帅的涛 on 2023/9/5.
//

#include "application.h"
#include "src/config.h"
#include "src/env.h"
#include "src/log.h"
#include "src/daemon.h"
#include <unistd.h>

namespace dht {

static dht::Logger::ptr g_logger = DHT_LOG_NAME("system");

static dht::ConfigVar<std::string>::ptr g_server_work_path =
        dht::Config::Lookup("server.work_path"
                ,std::string("/apps/work/dht")
                , "server work path");

static dht::ConfigVar<std::string>::ptr g_server_pid_file =
        dht::Config::Lookup("server.pid_file"
                ,std::string("dht.pid")
                , "server pid file");

struct HttpServerConf {
    std::vector<std::string> address;
    int keepalive = 0;
    int timeout = 1000 * 2 * 60;
    int ssl = 0;
    std::string name;
    std::string cert_file;
    std::string key_file;

    bool isValid() const {
        return !address.empty();
    }

    bool operator==(const HttpServerConf& oth) const {
        return address == oth.address
               && keepalive == oth.keepalive
               && timeout == oth.timeout
               && name == oth.name
               && ssl == oth.ssl
               && cert_file == oth.cert_file
               && key_file == oth.key_file;
    }
};

template<>
class LexicalCast<std::string, HttpServerConf> {
public:
    HttpServerConf operator()(const std::string& v) {
        YAML::Node node = YAML::Load(v);
        HttpServerConf conf;
        conf.keepalive = node["keepalive"].as<int>(conf.keepalive);
        conf.timeout = node["timeout"].as<int>(conf.timeout);
        conf.name = node["name"].as<std::string>(conf.name);
        conf.ssl = node["ssl"].as<int>(conf.ssl);
        conf.name = node["cert_file"].as<std::string>(conf.cert_file);
        conf.name = node["key_file"].as<std::string>(conf.key_file);
        if(node["address"].IsDefined()) {
            for(size_t i = 0; i < node["address"].size(); ++i) {
                conf.address.push_back(node["address"][i].as<std::string>());
            }
        }
        return conf;
    }
};

template<>
class LexicalCast<HttpServerConf, std::string> {
public:
    std::string operator()(const HttpServerConf& conf) {
        YAML::Node node;
        node["name"] = conf.name;
        node["keepalive"] = conf.keepalive;
        node["timeout"] = conf.timeout;
        node["ssl"] = conf.ssl;
        node["cert_file"] = conf.cert_file;
        node["key_file"] = conf.key_file;
        for(auto& i : conf.address) {
            node["address"].push_back(i);
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

static dht::ConfigVar<std::vector<HttpServerConf> >::ptr g_http_servers_conf
        = dht::Config::Lookup("http_servers", std::vector<HttpServerConf>(), "http server config");

Application* Application::s_instance = nullptr;

Application::Application() {
    s_instance = this;
}

bool Application::init(int argc, char** argv) {
    m_argc = argc;
    m_argv = argv;

    dht::EnvMgr::GetInstance()->addHelp("s", "start with the terminal");
    dht::EnvMgr::GetInstance()->addHelp("d", "run as daemon");
    dht::EnvMgr::GetInstance()->addHelp("c", "conf path default: ./conf");
    dht::EnvMgr::GetInstance()->addHelp("p", "print help");

    if(!dht::EnvMgr::GetInstance()->init(argc, argv)) {
        dht::EnvMgr::GetInstance()->printHelp();
        return false;
    }

    if(dht::EnvMgr::GetInstance()->has("p")) {
        dht::EnvMgr::GetInstance()->printHelp();
        return false;
    }

    int run_type = 0;
    if(dht::EnvMgr::GetInstance()->has("s")) {
        run_type = 1;
    }
    if(dht::EnvMgr::GetInstance()->has("d")) {
        run_type = 2;
    }

    if(run_type == 0) {
        dht::EnvMgr::GetInstance()->printHelp();
        return false;
    }

    std::string conf_path = dht::EnvMgr::GetInstance()->getAbsolutePath(
            dht::EnvMgr::GetInstance()->get("c", "conf")
    );
    DHT_LOG_INFO(g_logger) << "load conf path:" << conf_path;
    dht::Config::LoadFromConfDir(conf_path);

    std::string pidfile = g_server_work_path->getValue()
                          + "/" + g_server_pid_file->getValue();
    if(dht::FSUtil::IsRunningPidfile(pidfile)) {
        DHT_LOG_ERROR(g_logger) << "server is running:" << pidfile;
        return false;
    }

    if(!dht::FSUtil::Mkdir(g_server_work_path->getValue())) {
        DHT_LOG_FATAL(g_logger) << "create work path [" << g_server_work_path->getValue()
                                  << " errno=" << errno << " errstr=" << strerror(errno);
        return false;
    }
    return true;
}

bool Application::run() {
    bool is_daemon = dht::EnvMgr::GetInstance()->has("d");
    return start_daemon(m_argc, m_argv,
                        std::bind(&Application::main, this, std::placeholders::_1,
                                  std::placeholders::_2), is_daemon);
}

int Application::main(int argc, char** argv) {
    DHT_LOG_INFO(g_logger) << "main";
    {
        std::string pidfile = g_server_work_path->getValue()
                              + "/" + g_server_pid_file->getValue();
        std::ofstream ofs(pidfile);
        if(!ofs) {
            DHT_LOG_ERROR(g_logger) << "open pidfile " << pidfile << " failed";
            return false;
        }
        ofs << getpid();
    }

    dht::IOManager iom(1);
    iom.schedule(std::bind(&Application::run_fiber, this));
    iom.stop();
    return 0;
}

int Application::run_fiber() {
    auto http_confs = g_http_servers_conf->getValue();
    for(auto& i : http_confs) {
        DHT_LOG_INFO(g_logger) << LexicalCast<HttpServerConf, std::string>()(i);

        std::vector<Address::ptr> address;
        for(auto& a : i.address) {
            size_t pos = a.find(":");
            if(pos == std::string::npos) {
                //DHT_LOG_ERROR(g_logger) << "invalid address: " << a;
                address.push_back(UnixAddress::ptr(new UnixAddress(a)));
                continue;
            }
            int32_t port = atoi(a.substr(pos + 1).c_str());
            //127.0.0.1
            auto addr = dht::IPAddress::Create(a.substr(0, pos).c_str(), port);
            if(addr) {
                address.push_back(addr);
                continue;
            }
            std::vector<std::pair<Address::ptr, uint32_t> > result;
            if(!dht::Address::GetInterfaceAddresses(result,
                                                      a.substr(0, pos))) {
                DHT_LOG_ERROR(g_logger) << "invalid address: " << a;
                continue;
            }
            for(auto& x : result) {
                auto ipaddr = std::dynamic_pointer_cast<IPAddress>(x.first);
                if(ipaddr) {
                    ipaddr->setPort(atoi(a.substr(pos + 1).c_str()));
                }
                address.push_back(ipaddr);
            }
        }
        dht::http::HttpServer::ptr server(new dht::http::HttpServer(i.keepalive));
        std::vector<Address::ptr> fails;
        if(!server->bind(address, fails, i.ssl)) {
            for(auto& x : fails) {
                DHT_LOG_ERROR(g_logger) << "bind address fail:"
                                          << *x;
            }
            _exit(0);
        }
        if(i.ssl) {
            if(!server->loadCertificates(i.cert_file, i.key_file)) {
                DHT_LOG_ERROR(g_logger) << "loadCertificates fail, cert_file="
                                          << i.cert_file << " key_file=" << i.key_file;
            }
        }
        if(!i.name.empty()) {
            server->setName(i.name);
        }
        server->start();
        m_httpservers.push_back(server);

    }

    while(true) {
        DHT_LOG_INFO(g_logger) << "hello world";
        usleep(1000 * 100);
    }
    return 0;
}

}

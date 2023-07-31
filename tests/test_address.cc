//
// Created by 帅帅的涛 on 2023/7/31.
//

#include "src/address.h"
#include "src/log.h"

dht::Logger::ptr g_logger = DHT_LOG_ROOT();

void test(){
    std::vector<dht::Address::ptr> addrs;

    bool v = dht::Address::Lookup(addrs,"www.baidu.com");
    if(!v) {
        DHT_LOG_ERROR(g_logger) << "lookup fail";
        return;
    }

    for(size_t i = 0; i < addrs.size(); ++i){
        DHT_LOG_INFO(g_logger) << i << " - " << addrs[i]->toString();
    }
}

void test_iface() {
    std::multimap<std::string, std::pair<dht::Address::ptr, uint32_t>> results;
    bool v = dht::Address::GetInterfaceAddresses(results);
    if(!v) {
        DHT_LOG_ERROR(g_logger) << "GetInterfaceAddresses fail";
        return;
    }

    for(auto& i : results){
        DHT_LOG_INFO(g_logger) << i.first << "-" << i.second.first->toString() << "-"
                << i.second.second;
    }
}

void test_ipv4() {
    //auto addr = dht::IPAddress::Create("www.sylar.top");
    auto addr = dht::IPAddress::Create("127.0.0.8");
    if(addr) {
        DHT_LOG_INFO(g_logger) << addr->toString();
    }
}

int main(int argc, char** argv){
    //test();
    //test_iface();
    test_ipv4();
    return 0;
}
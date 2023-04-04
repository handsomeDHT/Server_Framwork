//
// Created by 帅帅的涛 on 2023/4/4.
//

#include "../src/config.h"
#include "../src/log.h"

dht::ConfigVar<int>::ptr g_int_value_config =
        dht::Config::Lookup("system.port", (int)8080, "system prot");

int main(int argc, char** argv){
    //dht::Logger::ptr logger;
    DHT_LOG_INFO(DHT_LOG_ROOT()) << g_int_value_config->getValue();
    DHT_LOG_INFO(DHT_LOG_ROOT()) << g_int_value_config->toString();
    return 0;
}
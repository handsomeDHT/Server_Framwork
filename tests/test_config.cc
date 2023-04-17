//
// Created by 帅帅的涛 on 2023/4/4.
//

#include "../src/config.h"
#include "../src/log.h"
#include <yaml-cpp/yaml.h>

dht::ConfigVar<int>::ptr g_int_value_config =
        dht::Config::Lookup(
                "system.port"
                ,(int)8080 //m_val
                ,"system port");

dht::ConfigVar<float>::ptr g_float_value_config =
        dht::Config::Lookup(
                "system.value"
                ,(float)10.2f //m_val
                ,"system value");

dht::ConfigVar<std::vector<int>>::ptr g_int_vec_value_config =
        dht::Config::Lookup(
                "system.int_vec"
                ,std::vector<int>{1,2} //m_val
                ,"system int vec");

void print_yaml(const YAML::Node& node, int level) {
    //根据不同的yaml类型进行解析
    if(node.IsScalar()) {
        DHT_LOG_INFO(DHT_LOG_ROOT()) << std::string(level * 4, ' ')
                                         << node.Scalar() << " - " << node.Type() << " - " << level;
    } else if(node.IsNull()) {
        DHT_LOG_INFO(DHT_LOG_ROOT()) << std::string(level * 4, ' ')
                                         << "NULL - " << node.Type() << " - " << level;
    } else if(node.IsMap()) {
        for(auto it = node.begin();
            it != node.end(); ++it) {
            DHT_LOG_INFO(DHT_LOG_ROOT()) << std::string(level * 4, ' ')
                                             << it->first << " - " << it->second.Type() << " - " << level;
            print_yaml(it->second, level + 1);
        }
    } else if(node.IsSequence()) {
        for(size_t i = 0; i < node.size(); ++i) {
            DHT_LOG_INFO(DHT_LOG_ROOT()) << std::string(level * 4, ' ')
                                             << i << " - " << node[i].Type() << " - " << level;
            print_yaml(node[i], level + 1);
        }
    }
}

void test_config(){
    DHT_LOG_INFO(DHT_LOG_ROOT()) << "before:" << g_int_value_config->getValue();
    DHT_LOG_INFO(DHT_LOG_ROOT()) << "before:" << g_float_value_config->toString();
    auto& v = g_int_vec_value_config->getValue();
    for(auto& i : v){
        DHT_LOG_INFO(DHT_LOG_ROOT()) << "int_vec:" << i;
    }

    //加载配置项，改变日志信息。
    YAML::Node root = YAML::LoadFile("/home/Server_Framwork/bin/conf/log.yml");
    dht::Config::LoadFromYaml(root);

    DHT_LOG_INFO(DHT_LOG_ROOT()) << "after:" << g_int_value_config->getValue();
    DHT_LOG_INFO(DHT_LOG_ROOT()) << "after:" << g_float_value_config->toString();
}

void test_yaml(){
    YAML::Node root = YAML::LoadFile("/home/Server_Framwork/bin/conf/log.yml");
    //加载log.yml后，进行遍历解析
    print_yaml(root, 0);

    //DHT_LOG_INFO(DHT_LOG_ROOT()) << root;
}

int main(int argc, char** argv){
    test_config();
    //test_yaml();

    return 0;
}
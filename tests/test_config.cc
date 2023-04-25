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

dht::ConfigVar<std::list<int>>::ptr g_int_list_value_config =
        dht::Config::Lookup(
                "system.int_list"
                ,std::list<int>{1,2} //m_val
                ,"system int list");

dht::ConfigVar<std::set<int>>::ptr g_int_set_value_config =
        dht::Config::Lookup(
                "system.int_set"
                ,std::set<int>{1,2} //m_val
                ,"system int set");

dht::ConfigVar<std::unordered_set<int>>::ptr g_int_uset_value_config =
        dht::Config::Lookup(
                "system.int_uset"
                ,std::unordered_set<int>{1,2} //m_val
                ,"system int uset");

dht::ConfigVar<std::map<std::string, int>>::ptr g_int_map_value_config =
        dht::Config::Lookup(
                "system.int_map"
                ,std::map<std::string, int>{{"k",2}} //m_val
                ,"system int map");

dht::ConfigVar<std::unordered_map<std::string, int>>::ptr g_int_umap_value_config =
        dht::Config::Lookup(
                "system.int_umap"
                ,std::unordered_map<std::string, int>{{"k",2}} //m_val
                ,"system int umap");

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

#define XX(g_var, name, prefix) \
    { \
        auto& v = g_var->getValue(); \
        for(auto& i : v) { \
            DHT_LOG_INFO(DHT_LOG_ROOT()) << #prefix " " #name ": " << i; \
        } \
        DHT_LOG_INFO(DHT_LOG_ROOT()) << #prefix "  " #name " yaml: " << g_var->toString(); \
    }


#define XX_M(g_var, name, prefix) \
    { \
        auto& v = g_var->getValue(); \
        for(auto& i : v) { \
            DHT_LOG_INFO(DHT_LOG_ROOT()) << #prefix " " #name ": {" \
                    << i.first << " - " << i.second << "}"; \
        } \
        DHT_LOG_INFO(DHT_LOG_ROOT()) << #prefix " " #name " yaml: " << g_var->toString(); \
    }

    XX(g_int_vec_value_config, int_vec, before);
    XX(g_int_list_value_config, int_list, before);
    XX(g_int_set_value_config, int_set, before);
    XX(g_int_uset_value_config, int_uset, before);
    XX_M(g_int_map_value_config, int_map, before);
    XX_M(g_int_umap_value_config, int_umap, before);

    //加载配置项，改变日志信息。
    YAML::Node root = YAML::LoadFile("/home/Server_Framwork/bin/conf/test.yml");
    dht::Config::LoadFromYaml(root);

    DHT_LOG_INFO(DHT_LOG_ROOT()) << "after: " << g_int_value_config->getValue();
    DHT_LOG_INFO(DHT_LOG_ROOT()) << "after: " << g_float_value_config->toString();

    XX(g_int_vec_value_config, int_vec, after);
    XX(g_int_list_value_config, int_list, after);
    XX(g_int_set_value_config, int_set, after);
    XX(g_int_uset_value_config, int_uset, after);
    XX_M(g_int_map_value_config, str_int_map, after);
    XX_M(g_int_umap_value_config, str_int_umap, after);
}

void test_yaml(){
    YAML::Node root = YAML::LoadFile("/home/Server_Framwork/bin/conf/test.yml");
    //加载log.yml后，进行遍历解析
    print_yaml(root, 0);

    //DHT_LOG_INFO(DHT_LOG_ROOT()) << root;
}

class Person{
public:
    Person(){};
    std::string m_name;
    int m_age = 0;
    bool m_sex = 0;

    std::string toString() const{
        std::stringstream ss;
        ss << "[Person name=" << m_name
                << " age=" << m_age
                << " sex=" << m_sex
                << "]";
        return ss.str();
    }
    bool operator==(const Person& oth) const {
        return m_name == oth.m_name
               && m_age == oth.m_age
               && m_sex == oth.m_sex;
    }
};

namespace dht {
template<>
class LexicalCast<std::string, Person> {
public:
    Person operator()(const std::string &v) {
        YAML::Node node = YAML::Load(v);
        Person p;
        p.m_name = node["name"].as<std::string>();
        p.m_age = node["age"].as<int>();
        p.m_sex = node["sex"].as<bool>();
        return p;
    }
};

template<>
class LexicalCast<Person, std::string> {
public:
    std::string operator()(const Person &p) {
        YAML::Node node;
        node["name"] = p.m_name;
        node["age"] = p.m_age;
        node["sex"] = p.m_sex;
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

}

dht::ConfigVar<Person>::ptr g_person =
        dht::Config::Lookup(
                "class.person"
                ,Person() //m_val
                ,"system person");

dht::ConfigVar<std::map<std::string, Person> >::ptr g_person_map =
        dht::Config::Lookup(
                "class.map"
                , std::map<std::string, Person>()
                , "system person");

dht::ConfigVar<std::map<std::string, std::vector<Person> > >::ptr g_person_vec_map =
        dht::Config::Lookup(
                "class.vec_map"
                , std::map<std::string
                , std::vector<Person> >()
                , "system person");

void test_class() {

    g_person->addListener(10, [](const Person& old_value, const Person& new_value){
        DHT_LOG_INFO(DHT_LOG_ROOT()) << "old_value=" << old_value.toString()
                                     << " new_value=" << new_value.toString();
    });

    DHT_LOG_INFO(DHT_LOG_ROOT()) << "before: " << g_person->getValue().toString() << " - " << g_person->toString();

#define XX_PM(g_var, prefix) \
    { \
        auto m = g_person_map->getValue(); \
        for(auto& i : m) { \
            DHT_LOG_INFO(DHT_LOG_ROOT()) <<  prefix << ": " << i.first << " - " << i.second.toString(); \
        } \
        DHT_LOG_INFO(DHT_LOG_ROOT()) <<  prefix << ": size=" << m.size(); \
    }

    XX_PM(g_person_map, "class.map before");
    DHT_LOG_INFO(DHT_LOG_ROOT()) << "before: " << g_person_vec_map->toString();

    YAML::Node root = YAML::LoadFile("/home/Server_Framwork/bin/conf/test.yml");
    dht::Config::LoadFromYaml(root);

    DHT_LOG_INFO(DHT_LOG_ROOT()) << "after: "
            << g_person->getValue().toString()
            << " - " << g_person->toString();

    XX_PM(g_person_map, "class.map after");
    DHT_LOG_INFO(DHT_LOG_ROOT()) << "after: " << g_person_vec_map->toString();

}


int main(int argc, char** argv){
    //test_config();
    //test_yaml();
    test_class();


    return 0;
}
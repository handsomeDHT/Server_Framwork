//
// Created by 帅帅的涛 on 2023/4/4.
//

#include "config.h"

namespace dht{

//ConfigVarBase::ConfigVarBase(const std::string &name, const std::string &description) {}

ConfigVarBase::ptr Config::LookupBase(const std::string &name) {
    RWMutexType::ReadLock lock(GetMutex());
    auto it = GetDatas().find(name);
    return it == GetDatas().end()? nullptr : it->second;
}

/**
 * @brief 读取yml文件中的配置信息
 * @param prefix
 * 逐层递归进行遍历，并将遍历出来的压入output
 */
static void ListAllMember (const std::string& prefix
                           ,const YAML::Node& node
                           ,std::list<std::pair<std::string ,const YAML::Node>>& output){
    if(prefix.find_first_not_of("abcdefghikjlmnopqrstuvwxyz._012345678")
                != std::string::npos ){
        DHT_LOG_ERROR(DHT_LOG_ROOT()) << "Config invalid name:" << prefix << ":" << node;
        return;
    }

    output.emplace_back(prefix, node);
    if(node.IsMap()){
        for(auto it = node.begin();
                it != node.end(); ++it){
            ListAllMember(prefix.empty() ? it->first.Scalar()
                          : prefix + "." + it->first.Scalar()
                          , it->second
                          , output);
        }
    }
}
/**
 * @brief 将加载的yml配置项写入log
 * @param root
 */
void Config::LoadFromYaml(const YAML::Node &root) {
    std::list<std::pair<std::string, const YAML::Node>> all_nodes;
    //将log.yml中的配置递归读取出来
    ListAllMember("", root, all_nodes);

    for(auto& i : all_nodes){
        std::string key = i.first;

        if(key.empty()){
            continue;
        }

        //std::transform：在给定范围内应用于给定操作，并将结果存储在指定的另一个范围内
        std::transform(key.begin()
                       , key.end()
                       , key.begin()
                       , ::tolower);

        ConfigVarBase::ptr var = LookupBase(key);
        //将从log.yml中的信息加载到log配置信息中去
        if(var){
            if(i.second.IsScalar()){
                var->fromString(i.second.Scalar());
            }else {
                std::stringstream ss;
                ss << i.second;
                var -> fromString(ss.str());
            }
        }
    }
}

void Config::Visit(std::function<void(ConfigVarBase::ptr)> cb) {
    RWMutexType::ReadLock lock(GetMutex());
    ConfigVarMap& m = GetDatas();
    for(auto it = m.begin();
        it != m.end(); ++it) {
        cb(it->second);
    }
}


}
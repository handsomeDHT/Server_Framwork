/**
 * URI封装类
 */
#ifndef SERVER_FRAMWORK_URI_H
#define SERVER_FRAMWORK_URI_H


#include <memory>
#include <string>
#include <stdint.h>
#include "address.h"

namespace dht {

/*
     foo://user@sylar.com:8042/over/there?name=ferret#nose
       \_/   \______________/\_________/ \_________/ \__/
        |           |            |            |        |
     scheme     authority       path        query   fragment

 方案（Scheme）：标识了使用的协议或规范，例如 "http"、"https"、"ftp" 等。
主机（Host）：标识了资源所在的主机或服务器的域名或IP地址。
端口（Port）：可选组件，标识了与主机通信的端口号。
路径（Path）：标识了资源在服务器上的位置，通常表示为一个文件路径或路由。
查询（Query）：可选组件，包含了向服务器请求资源时的参数。
片段（Fragment）：可选组件，标识了资源中的特定部分。
*/

/**
 * @brief URI类
 */
class Uri {
public:
    /// 智能指针类型定义
    typedef std::shared_ptr<Uri> ptr;

    /**
     * @brief 创建Uri对象
     * @param uri uri字符串
     * @return 解析成功返回Uri对象否则返回nullptr
     */
    static Uri::ptr Create(const std::string& uri);

    /**
     * @brief 构造函数
     */
    Uri();

    /**
     * @brief 返回scheme
     */
    const std::string& getScheme() const { return m_scheme;}

    /**
     * @brief 返回用户信息
     */
    const std::string& getUserinfo() const { return m_userinfo;}

    /**
     * @brief 返回host
     */
    const std::string& getHost() const { return m_host;}

    /**
     * @brief 返回路径
     */
    const std::string& getPath() const;

    /**
     * @brief 返回查询条件
     */
    const std::string& getQuery() const { return m_query;}

    /**
     * @brief 返回fragment
     */
    const std::string& getFragment() const { return m_fragment;}

    /**
     * @brief 返回端口
     */
    int32_t getPort() const;

    /**
     * @brief 设置scheme
     * @param v scheme
     */
    void setScheme(const std::string& v) { m_scheme = v;}

    /**
     * @brief 设置用户信息
     * @param v 用户信息
     */
    void setUserinfo(const std::string& v) { m_userinfo = v;}

    /**
     * @brief 设置host信息
     * @param v host
     */
    void setHost(const std::string& v) { m_host = v;}

    /**
     * @brief 设置路径
     * @param v 路径
     */
    void setPath(const std::string& v) { m_path = v;}

    /**
     * @brief 设置查询条件
     * @param v
     */
    void setQuery(const std::string& v) { m_query = v;}

    /**
     * @brief 设置fragment
     * @param v fragment
     */
    void setFragment(const std::string& v) { m_fragment = v;}

    /**
     * @brief 设置端口号
     * @param v 端口
     */
    void setPort(int32_t v) { m_port = v;}

    /**
     * @brief 序列化到输出流
     * @param os 输出流
     * @return 输出流
     */
    std::ostream& dump(std::ostream& os) const;

    /**
     * @brief 转成字符串
     */
    std::string toString() const;

    /**
     * @brief 获取Address
     */
    Address::ptr createAddress() const;
private:

    /**
     * @brief 是否默认端口
     */
    bool isDefaultPort() const;
private:
    /// schema
    std::string m_scheme;
    /// 用户信息
    std::string m_userinfo;
    /// host
    std::string m_host;
    /// 路径
    std::string m_path;
    /// 查询参数
    std::string m_query;
    /// fragment
    std::string m_fragment;
    /// 端口
    int32_t m_port;
};

}


#endif //SERVER_FRAMWORK_URI_H

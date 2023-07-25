//
// Created by 帅帅的涛 on 2023/7/25.
// 网络地址封装
//

#ifndef SERVER_FRAMWORK_ADDRESS_H
#define SERVER_FRAMWORK_ADDRESS_H

#include <memory>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <vector>
#include <map>

namespace dht{

//基类
class Address{
public:
    typedef std::shared_ptr<Address> ptr;
    virtual ~Address(){}

    int getFamily() const;
    virtual const sockaddr* getAddr() const = 0;
    virtual socklen_t getAddrLen() const = 0;
    virtual std::ostream& insert(std::ostream& os) const = 0;

    std::string toString();

    bool operator< (const Address& rhs) const;
    bool operator==(const Address& rhs) const;
    bool operator!=(const Address& rhs) const;
};

class IPAddress : public Address{
public:
    typedef std::shared_ptr<IPAddress> ptr;

    virtual IPAddress::ptr broadcastAddress(uint32_t prefix_len) = 0;
    virtual IPAddress::ptr networdAddress(uint32_t prefix_len) = 0;
    virtual IPAddress::ptr subnetMask(uint32_t prefix_len) = 0;

    virtual uint32_t getPort() const = 0;
    virtual void setPPort(uint32_t v) = 0;
};

class IPv4Address : public IPAddress{
public:
    typedef std::shared_ptr<IPv4Address> ptr;
    IPv4Address(uint32_t address = INADDR_ANY, uint32_t port = 0);

    const sockaddr* getAddr() const override;
    socklen_t getAddrLen() const override;
    std::ostream& insert(std::ostream& os) const override;

    IPAddress::ptr broadcastAddress(uint32_t prefix_len) override;
    IPAddress::ptr networdAddress(uint32_t prefix_len) override;
    IPAddress::ptr subnetMask(uint32_t prefix_len) override;

    uint32_t getPort() const override;
    void setPPort(uint32_t v) override;
private:
    sockaddr_in m_addr;
};

class IPv6Address : public IPAddress{
public:
    typedef std::shared_ptr<IPv6Address> ptr;
    IPv6Address(uint32_t address = INADDR_ANY, uint32_t port = 0);

    const sockaddr* getAddr() const override;
    socklen_t getAddrLen() const override;
    std::ostream& insert(std::ostream& os) const override;

    IPAddress::ptr broadcastAddress(uint32_t prefix_len) override;
    IPAddress::ptr networdAddress(uint32_t prefix_len) override;
    IPAddress::ptr subnetMask(uint32_t prefix_len) override;

    uint32_t getPort() const override;
    void setPPort(uint32_t v) override;
private:
    sockaddr_in6 m_addr;
};

class UnixAddress :public Address{
public:
    typedef std::shared_ptr<UnixAddress> ptr;
    explicit UnixAddress(const std::string& path);

    const sockaddr* getAddr() const override;
    socklen_t getAddrLen() const override;
    std::ostream& insert(std::ostream& os) const override;
private:
    sockaddr_un  m_addr;
    socklen_t m_length;
};

class UnknowAddress : public Address{
public:
    typedef std::shared_ptr<UnknowAddress> ptr;
    UnknowAddress();
    const sockaddr* getAddr() const override;
    socklen_t getAddrLen() const override;
    std::ostream& insert(std::ostream& os) const override;
private:
    sockaddr m_addr;
};

}

#endif //SERVER_FRAMWORK_ADDRESS_H

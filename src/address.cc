//
// Created by 帅帅的涛 on 2023/7/25.
//

#include "address.h"
#include <sstream>

namespace dht{
/**
 * class Address
 * ---------------------------------------------------------------
 */
int Address::getFamily() const {
    return getAddr()->sa_family;
}

std::string Address::toString() {
    std::stringstream ss;
    insert(ss);
    return ss.str();
}
bool Address::operator<(const Address &rhs) const {
    socklen_t minlen = std::min(getAddrLen(), rhs.getAddrLen());
    int result = memcmp(getAddr(), rhs.getAddr(), minlen);
    if(result < 0) {
        return true;
    } else if (result < 0) {
        return false;
    } else if(getAddrLen() < rhs.getAddrLen()) {
        return true;
    };
    return false;
}

bool Address::operator==(const Address &rhs) const {
    return getAddrLen() == rhs.getAddrLen()
           && memcmp(getAddr(), rhs.getAddr(), getAddrLen()) == 0;
}

bool Address::operator!=(const Address &rhs) const {
    return !(*this == rhs);
}

/**
 * class IPv4Address
 * ---------------------------------------------------------------
 */

IPv4Address::IPv4Address(uint32_t address, uint32_t port) {

}

const sockaddr *IPv4Address::getAddr() const {
    return nullptr;
}

socklen_t IPv4Address::getAddrLen() const {
    return 0;
}

std::ostream &IPv4Address::insert(std::ostream &os) const {

}

IPAddress::ptr IPv4Address::broadcastAddress(uint32_t prefix_len) {

}

IPAddress::ptr IPv4Address::networdAddress(uint32_t prefix_len) {

}

IPAddress::ptr IPv4Address::subnetMask(uint32_t prefix_len) {

}

uint32_t IPv4Address::getPort() const {

}

void IPv4Address::setPPort(uint32_t v) {

}

/**
 * class IPv6Address
 * ---------------------------------------------------------------
 */

IPv6Address::IPv6Address(uint32_t address, uint32_t port) {

}

const sockaddr *IPv6Address::getAddr() const {

}

socklen_t IPv6Address::getAddrLen() const {

}

std::ostream &IPv6Address::insert(std::ostream &os) const {

}

IPAddress::ptr IPv6Address::broadcastAddress(uint32_t prefix_len) {

}

IPAddress::ptr IPv6Address::networdAddress(uint32_t prefix_len) {

}

IPAddress::ptr IPv6Address::subnetMask(uint32_t prefix_len) {

}

uint32_t IPv6Address::getPort() const {

}

void IPv6Address::setPPort(uint32_t v) {

}

/**
 * class UnixAddress
 * ---------------------------------------------------------------
 */
UnixAddress::UnixAddress(const std::string &path) {

}

const sockaddr *UnixAddress::getAddr() const {

}

socklen_t UnixAddress::getAddrLen() const {

}

std::ostream &UnixAddress::insert(std::ostream &os) const {

}

/**
 * class UnknowAddress
 * ---------------------------------------------------------------
 */

UnknowAddress::UnknowAddress() {

}

const sockaddr *UnknowAddress::getAddr() const {

}

socklen_t UnknowAddress::getAddrLen() const {

}

std::ostream &UnknowAddress::insert(std::ostream &os) const {

}
}
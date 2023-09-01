//
// Created by 帅帅的涛 on 2023/9/1.
//

#include "src/uri.h"
#include <iostream>

int main(int argc, char** argv) {
    //dht::Uri::ptr uri = dht::Uri::Create("http://www.dht.top/test/uri?id=100&name=dht#frg");
    //dht::Uri::ptr uri = dht::Uri::Create("http://admin@www.dht.top/test/中文/uri?id=100&name=dht&vv=中文#frg中文");
    dht::Uri::ptr uri = dht::Uri::Create("http://admin@www.sylar.top");
    //dht::Uri::ptr uri = dht::Uri::Create("http://www.dht.top/test/uri");
    std::cout << uri->toString() << std::endl;
    auto addr = uri->createAddress();
    std::cout << *addr << std::endl;
    return 0;
}
//
// Created by 帅帅的涛 on 2023/8/9.
//

#include "src/dht.h"

void test_request(){
    dht::http::HttpRequest::ptr req(new dht::http::HttpRequest);
    req->setHeader("host" , "www.baidu.com");
    req->setBody("hello baidu");

    req->dump(std::cout) << std::endl;
}

void test_response(){
    dht::http::HttpResponse::ptr rsp(new dht::http::HttpResponse);
    rsp->setHeader("X-X", " test ");
    rsp->setBody("hello test");
    rsp->setStatus((dht::http::HttpStatus)400);
    rsp->setClose(false);

    rsp->dump(std::cout) << std::endl;
}

int main(int argc, char** argv){
    test_request();
    test_response();
    return 0;
}
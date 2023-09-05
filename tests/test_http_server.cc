#include "src/http/http_server.h"
#include "src/log.h"

static dht::Logger::ptr g_logger = DHT_LOG_ROOT();

#define XX(...) #__VA_ARGS__


dht::IOManager::ptr worker;
void run() {
    g_logger->setLevel(dht::LogLevel::INFO);
    //dht::http::HttpServer::ptr server(new dht::http::HttpServer(true, worker.get(), dht::IOManager::GetThis()));
    // 创建一个HTTP服务器实例，长连接true
    dht::http::HttpServer::ptr server(new dht::http::HttpServer(true));
    dht::Address::ptr addr = dht::Address::LookupAnyIPAddress("0.0.0.0:8020");
    // 使用循环来绑定服务器到指定地址，如果绑定失败则每隔2秒重试
    while(!server->bind(addr)) {
        sleep(2);
    }
    // 获取HTTP服务器的Servlet分发器
    auto sd = server->getServletDispatch();
    // 添加一个Servlet，处理路径为/dht/xx的HTTP请求
    sd->addServlet("/dht/xx", [](dht::http::HttpRequest::ptr req
            ,dht::http::HttpResponse::ptr rsp
            ,dht::http::HttpSession::ptr session) {
        rsp->setBody(req->toString());
        return 0;
    });

    // 添加一个Glob Servlet，处理路径匹配/dht/*的HTTP请求
    sd->addGlobServlet("/dht/*", [](dht::http::HttpRequest::ptr req
            ,dht::http::HttpResponse::ptr rsp
            ,dht::http::HttpSession::ptr session) {
        rsp->setBody("Glob:\r\n" + req->toString());
        return 0;
    });

    // 添加另一个Glob Servlet，处理路径匹配/dhtx/*的HTTP请求
    sd->addGlobServlet("/dhtx/*", [](dht::http::HttpRequest::ptr req
            ,dht::http::HttpResponse::ptr rsp
            ,dht::http::HttpSession::ptr session) {
        rsp->setBody(XX(<html>
                                <head><title>404 Not Found test</title></head>
                                <body>
                                <center><h1>404 Not Found</h1></center>
                                <hr><center>nginx/1.16.0</center>
                                </body>
                                </html>
                                <!-- a padding to disable MSIE and Chrome friendly error page -->
                                <!-- a padding to disable MSIE and Chrome friendly error page -->
                                <!-- a padding to disable MSIE and Chrome friendly error page -->
                                <!-- a padding to disable MSIE and Chrome friendly error page -->
                                <!-- a padding to disable MSIE and Chrome friendly error page -->
                                <!-- a padding to disable MSIE and Chrome friendly error page -->
                     ));
        return 0;
    });

    server->start();
}

int main(int argc, char** argv) {
    dht::IOManager iom(1, true, "main");
    worker.reset(new dht::IOManager(3, false, "worker"));
    iom.schedule(run);
    return 0;
}
#include "src/http/http_server.h"
#include "src/log.h"

static dht::Logger::ptr g_logger = DHT_LOG_ROOT();

#define XX(...) #__VA_ARGS__


dht::IOManager::ptr worker;
void run() {
    g_logger->setLevel(dht::LogLevel::INFO);
    //dht::http::HttpServer::ptr server(new dht::http::HttpServer(true, worker.get(), dht::IOManager::GetThis()));
    dht::http::HttpServer::ptr server(new dht::http::HttpServer(true));
    dht::Address::ptr addr = dht::Address::LookupAnyIPAddress("0.0.0.0:8020");
    while(!server->bind(addr)) {
        sleep(2);
    }
    auto sd = server->getServletDispatch();
    sd->addServlet("/dht/xx", [](dht::http::HttpRequest::ptr req
            ,dht::http::HttpResponse::ptr rsp
            ,dht::http::HttpSession::ptr session) {
        rsp->setBody(req->toString());
        return 0;
    });

    sd->addGlobServlet("/dht/*", [](dht::http::HttpRequest::ptr req
            ,dht::http::HttpResponse::ptr rsp
            ,dht::http::HttpSession::ptr session) {
        rsp->setBody("Glob:\r\n" + req->toString());
        return 0;
    });

    sd->addGlobServlet("/dhtx/*", [](dht::http::HttpRequest::ptr req
            ,dht::http::HttpResponse::ptr rsp
            ,dht::http::HttpSession::ptr session) {
        rsp->setBody(XX(<html>
                                <head><title>404 Not Found</title></head>
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
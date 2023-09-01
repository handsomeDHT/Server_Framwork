//
// Created by 帅帅的涛 on 2023/9/1.
//

#ifndef SERVER_FRAMWORK_HTTP_CONNECTION_H
#define SERVER_FRAMWORK_HTTP_CONNECTION_H


#include "src/socket_stream.h"
#include "http.h"

namespace dht {
namespace http {

class HttpConnection : public SocketStream {
public:
    typedef std::shared_ptr<HttpConnection> ptr;
    HttpConnection(Socket::ptr sock, bool owner = true);
    HttpResponse::ptr recvResponse();
    int sendRequest(HttpRequest::ptr req);
};

}
}

#endif //SERVER_FRAMWORK_HTTP_CONNECTION_H

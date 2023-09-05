//
// Created by 帅帅的涛 on 2023/9/5.
//

#ifndef SERVER_FRAMWORK_WD_CONNECTION_H
#define SERVER_FRAMWORK_WD_CONNECTION_H

#include "src/http/http_connection.h"
#include "src/http/ws_session.h"

namespace dht {
namespace http {

class WSConnection : public HttpConnection {
public:
    typedef std::shared_ptr<WSConnection> ptr;
    WSConnection(Socket::ptr sock, bool owner = true);
    static std::pair<HttpResult::ptr, WSConnection::ptr> Create(const std::string& url
            ,uint64_t timeout_ms
            , const std::map<std::string, std::string>& headers = {});
    static std::pair<HttpResult::ptr, WSConnection::ptr> Create(Uri::ptr uri
            ,uint64_t timeout_ms
            , const std::map<std::string, std::string>& headers = {});
    WSFrameMessage::ptr recvMessage();
    int32_t sendMessage(WSFrameMessage::ptr msg, bool fin = true);
    int32_t sendMessage(const std::string& msg, int32_t opcode = WSFrameHead::TEXT_FRAME, bool fin = true);
    int32_t ping();
    int32_t pong();
};

}
}


#endif //SERVER_FRAMWORK_WD_CONNECTION_H

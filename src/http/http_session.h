//
// Created by 帅帅的涛 on 2023/8/29.
//

#ifndef SERVER_FRAMWORK_HTTP_SESSION_H
#define SERVER_FRAMWORK_HTTP_SESSION_H

#include "src/socket_stream.h"
#include "http.h"


namespace dht{
namespace http{

class HttpSession : public SocketStream{
public:
    typedef std::shared_ptr<HttpSession> ptr;
    /**
     * @brief 构造函数
     * @param[in] sock Socket类型
     * @param[in] owner 是否托管
     */
    HttpSession(Socket::ptr sock, bool owner = true);

    HttpRequest::ptr recvRequest();
    /**
     * @brief 发送HTTP响应
     * @param[in] rsp HTTP响应
     * @return >0 发送成功
     *         =0 对方关闭
     *         <0 Socket异常
     */
    int sendResponse(HttpResponse::ptr rsp);

};

}
}


#endif //SERVER_FRAMWORK_HTTP_SESSION_H

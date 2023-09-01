//
// Created by 帅帅的涛 on 2023/9/1.
//

#include "http_connection.h"
#include "http_parser.h"
#include "src/log.h"

#include "http_connection.h"
#include "http_parser.h"
#include "src/log.h"
#include "src/streams/zlib_stream.h"

namespace dht {
namespace http {

static dht::Logger::ptr g_logger = DHT_LOG_NAME("system");

std::string HttpResult::toString() const {
    std::stringstream ss;
    ss << "[HttpResult result=" << result
       << " error=" << error
       << " response=" << (response ? response->toString() : "nullptr")
       << "]";
    return ss.str();
}

HttpConnection::HttpConnection(Socket::ptr sock, bool owner)
        :SocketStream(sock, owner) {
}

HttpConnection::~HttpConnection() {
    DHT_LOG_DEBUG(g_logger) << "HttpConnection::~HttpConnection";
}

HttpResponse::ptr HttpConnection::recvResponse() {
    //HttpResponseParser -> HTTP自定义的解析结构体
    HttpResponseParser::ptr parser(new HttpResponseParser);
    uint64_t buff_size = HttpRequestParser::GetHttpRequestBufferSize();

    std::shared_ptr<char> buffer(
            new char[buff_size + 1], [](char* ptr){
                delete[] ptr;
            });
    char* data = buffer.get();
    int offset = 0;
    do {
        //从连接中获取数据到缓冲区
        int len = read(data + offset, buff_size - offset);
        // 如果读取的数据长度小于等于0，表示连接已关闭或出现错误
        if(len <= 0) {
            close();
            return nullptr;
        }
        // 更新数据长度，将缓冲区的数据长度与偏移相加
        len += offset;
        data[len] = '\0'; //在数据末尾添加空字符，以便处理字符串

        //使用解析器解析HTTP响应数据
        size_t nparse = parser->execute(data, len, false);
        // 检查解析器是否发生错误
        if(parser->hasError()) {
            close();
            return nullptr;
        }
        // 计算剩余未解析的数据长度
        offset = len - nparse;
        // 如果缓冲区已满，关闭连接并返回空指针
        if(offset == (int)buff_size) {
            close();
            return nullptr;
        }
        // 如果解析器已完成解析，跳出循环
        if(parser->isFinished()) {
            break;
        }
    } while(true);

    // 获取解析器的客户端解析器对象
    auto& client_parser = parser->getParser();
    std::string body;
    if(client_parser.chunked) {
        int len = offset; // 初始化数据长度为偏移值
        do {
            bool begin = true;// 标记是否是数据块的起始
            do {
                // 如果不是数据块的起始或者当前缓冲区为空，从连接中读取数据
                if(!begin || len == 0) {
                    int rt = read(data + len, buff_size - len);
                    if(rt <= 0) {
                        close();
                        return nullptr;
                    }
                    len += rt;// 更新数据长度
                }
                data[len] = '\0'; // 在数据末尾添加空字符，以便后续处理成字符串
                // 使用解析器解析HTTP响应数据块
                size_t nparse = parser->execute(data, len, true);
                // 检查解析器是否发生错误
                if(parser->hasError()) {
                    close();
                    return nullptr;
                }
                len -= nparse; // 计算剩余未解析的数据长度
                if(len == (int)buff_size) {
                    close();
                    return nullptr;
                }
                begin = false;
            } while(!parser->isFinished());
            //len -= 2;
            // 输出当前数据块的内容长度
            DHT_LOG_DEBUG(g_logger) << "content_len=" << client_parser.content_len;

            // 如果当前数据块内容可以完全添加到响应体中
            if(client_parser.content_len + 2 <= len) {
                // 将数据块内容添加到响应体中
                body.append(data, client_parser.content_len);
                // 移动缓冲区中剩余的数据，并更新长度
                memmove(data, data + client_parser.content_len + 2
                        , len - client_parser.content_len - 2);
                len -= client_parser.content_len + 2;
            } else {        // 如果当前数据块无法完全添加到响应体中
                body.append(data, len);
                // 计算剩余需要读取的数据长度
                int left = client_parser.content_len - len + 2;
                while(left > 0) {
                    // 从连接中读取剩余部分的数据
                    int rt = read(data, left > (int)buff_size ? (int)buff_size : left);
                    if(rt <= 0) {
                        close();
                        return nullptr;
                    }
                    body.append(data, rt); // 将读取的数据添加到响应体
                    left -= rt; // 更新剩余需要读取的数据长度
                }
                body.resize(body.size() - 2);// 移除结尾的两个字符（\r\n）
                len = 0;
            }
        } while(!client_parser.chunks_done); // 继续处理下一个数据块，直到数据块传输完毕
    } else {
        // 获取HTTP响应的内容长度（如果有）
        int64_t length = parser->getContentLength();
        if(length > 0) {
            // 调整响应体的大小以容纳内容
            body.resize(length);

            int len = 0;
            // 如果内容长度大于或等于偏移值（缓冲区中的数据），则将缓冲区中的数据复制到响应体中
            if(length >= offset) {
                memcpy(&body[0], data, offset);
                len = offset;
            } else {
                // 如果内容长度小于偏移值，只复制部分数据到响应体中
                memcpy(&body[0], data, length);
                len = length;
            }
            // 计算剩余未复制的内容长度
            length -= offset;
            if(length > 0) {
                // 如果仍有剩余的内容需要读取，继续从连接中读取
                if(readFixSize(&body[len], length) <= 0) {
                    close();
                    return nullptr;
                }
            }
        }
    }
    if(!body.empty()) {
        // 获取HTTP响应中的内容编码类型
        auto content_encoding = parser->getData()->getHeader("content-encoding");
        DHT_LOG_DEBUG(g_logger) << "content_encoding: " << content_encoding
                                  << " size=" << body.size();
        // 如果内容编码类型为gzip
        if(strcasecmp(content_encoding.c_str(), "gzip") == 0) {
            // 创建一个用于解压缩的ZlibStream对象，参数为false表示解压缩
            auto zs = ZlibStream::CreateGzip(false);
            // 将响应体内容写入解压缩流
            zs->write(body.c_str(), body.size());
            // 刷新解压缩流
            zs->flush();
            // 将解压缩后的结果替换掉原始的响应体内容
            zs->getResult().swap(body);
        } else if(strcasecmp(content_encoding.c_str(), "deflate") == 0) {   // 如果内容编码类型为deflate
            // 创建一个用于解压缩的ZlibStream对象，参数为false表示解压缩
            auto zs = ZlibStream::CreateDeflate(false);
            // 将响应体内容写入解压缩流
            zs->write(body.c_str(), body.size());
            // 刷新解压缩流
            zs->flush();
            // 将解压缩后的结果替换掉原始的响应体内容
            zs->getResult().swap(body);
        }
        parser->getData()->setBody(body);
    }
    return parser->getData();
}

int HttpConnection::sendRequest(HttpRequest::ptr rsp) {
    std::stringstream ss;
    ss << *rsp;
    std::string data = ss.str();
    //std::cout << ss.str() << std::endl;
    return writeFixSize(data.c_str(), data.size());
}

HttpResult::ptr HttpConnection::DoGet(const std::string& url
        , uint64_t timeout_ms
        , const std::map<std::string, std::string>& headers
        , const std::string& body) {
    Uri::ptr uri = Uri::Create(url);
    if(!uri) {
        return std::make_shared<HttpResult>((int)HttpResult::Error::INVALID_URL
                , nullptr, "invalid url: " + url);
    }
    return DoGet(uri, timeout_ms, headers, body);
}

HttpResult::ptr HttpConnection::DoGet(Uri::ptr uri
        , uint64_t timeout_ms
        , const std::map<std::string, std::string>& headers
        , const std::string& body) {
    return DoRequest(HttpMethod::GET, uri, timeout_ms, headers, body);
}

HttpResult::ptr HttpConnection::DoPost(const std::string& url
        , uint64_t timeout_ms
        , const std::map<std::string, std::string>& headers
        , const std::string& body) {
    Uri::ptr uri = Uri::Create(url);
    if(!uri) {
        return std::make_shared<HttpResult>((int)HttpResult::Error::INVALID_URL
                , nullptr, "invalid url: " + url);
    }
    return DoPost(uri, timeout_ms, headers, body);
}

HttpResult::ptr HttpConnection::DoPost(Uri::ptr uri
        , uint64_t timeout_ms
        , const std::map<std::string, std::string>& headers
        , const std::string& body) {
    return DoRequest(HttpMethod::POST, uri, timeout_ms, headers, body);
}

HttpResult::ptr HttpConnection::DoRequest(HttpMethod method
        , const std::string& url
        , uint64_t timeout_ms
        , const std::map<std::string, std::string>& headers
        , const std::string& body) {
    Uri::ptr uri = Uri::Create(url);
    if(!uri) {
        return std::make_shared<HttpResult>((int)HttpResult::Error::INVALID_URL
                , nullptr, "invalid url: " + url);
    }
    return DoRequest(method, uri, timeout_ms, headers, body);
}

HttpResult::ptr HttpConnection::DoRequest(HttpMethod method
        , Uri::ptr uri
        , uint64_t timeout_ms
        , const std::map<std::string, std::string>& headers
        , const std::string& body) {
    HttpRequest::ptr req = std::make_shared<HttpRequest>();
    req->setPath(uri->getPath());
    req->setQuery(uri->getQuery());
    req->setFragment(uri->getFragment());
    req->setMethod(method);
    bool has_host = false;
    for(auto& i : headers) {
        if(strcasecmp(i.first.c_str(), "connection") == 0) {
            if(strcasecmp(i.second.c_str(), "keep-alive") == 0) {
                req->setClose(false);
            }
            continue;
        }

        if(!has_host && strcasecmp(i.first.c_str(), "host") == 0) {
            has_host = !i.second.empty();
        }

        req->setHeader(i.first, i.second);
    }
    if(!has_host) {
        req->setHeader("Host", uri->getHost());
    }
    req->setBody(body);
    return DoRequest(req, uri, timeout_ms);
}

HttpResult::ptr HttpConnection::DoRequest(HttpRequest::ptr req
        , Uri::ptr uri
        , uint64_t timeout_ms) {
    bool is_ssl = uri->getScheme() == "https";
    Address::ptr addr = uri->createAddress();
    if(!addr) {
        return std::make_shared<HttpResult>((int)HttpResult::Error::INVALID_HOST
                , nullptr, "invalid host: " + uri->getHost());
    }
    Socket::ptr sock = is_ssl ? SSLSocket::CreateTCP(addr) : Socket::CreateTCP(addr);
    if(!sock) {
        return std::make_shared<HttpResult>((int)HttpResult::Error::CREATE_SOCKET_ERROR
                , nullptr, "create socket fail: " + addr->toString()
                           + " errno=" + std::to_string(errno)
                           + " errstr=" + std::string(strerror(errno)));
    }
    if(!sock->connect(addr)) {
        return std::make_shared<HttpResult>((int)HttpResult::Error::CONNECT_FAIL
                , nullptr, "connect fail: " + addr->toString());
    }
    sock->setRecvTimeout(timeout_ms);
    HttpConnection::ptr conn = std::make_shared<HttpConnection>(sock);
    int rt = conn->sendRequest(req);
    if(rt == 0) {
        return std::make_shared<HttpResult>((int)HttpResult::Error::SEND_CLOSE_BY_PEER
                , nullptr, "send request closed by peer: " + addr->toString());
    }
    if(rt < 0) {
        return std::make_shared<HttpResult>((int)HttpResult::Error::SEND_SOCKET_ERROR
                , nullptr, "send request socket error errno=" + std::to_string(errno)
                           + " errstr=" + std::string(strerror(errno)));
    }
    auto rsp = conn->recvResponse();
    if(!rsp) {
        return std::make_shared<HttpResult>((int)HttpResult::Error::TIMEOUT
                , nullptr, "recv response timeout: " + addr->toString()
                           + " timeout_ms:" + std::to_string(timeout_ms));
    }
    return std::make_shared<HttpResult>((int)HttpResult::Error::OK, rsp, "ok");
}

HttpConnectionPool::ptr HttpConnectionPool::Create(const std::string& uri
        ,const std::string& vhost
        ,uint32_t max_size
        ,uint32_t max_alive_time
        ,uint32_t max_request) {
    Uri::ptr turi = Uri::Create(uri);
    if(!turi) {
        DHT_LOG_ERROR(g_logger) << "invalid uri=" << uri;
    }
    return std::make_shared<HttpConnectionPool>(turi->getHost()
            , vhost, turi->getPort(), turi->getScheme() == "https"
            , max_size, max_alive_time, max_request);
}

//HttpConnectionPool___________----------------------------------------------------

HttpConnectionPool::HttpConnectionPool(const std::string& host
        ,const std::string& vhost
        ,uint32_t port
        ,bool is_https
        ,uint32_t max_size
        ,uint32_t max_alive_time
        ,uint32_t max_request)
        :m_host(host)
        ,m_vhost(vhost)
        ,m_port(port ? port : (is_https ? 443 : 80))
        ,m_maxSize(max_size)
        ,m_maxAliveTime(max_alive_time)
        ,m_maxRequest(max_request)
        ,m_isHttps(is_https) {
}

HttpConnection::ptr HttpConnectionPool::getConnection() {
    uint64_t now_ms = dht::GetCurrentMS();
    std::vector<HttpConnection*> invalid_conns;
    HttpConnection* ptr = nullptr;
    MutexType::Lock lock(m_mutex);
    while(!m_conns.empty()) {
        auto conn = *m_conns.begin();
        m_conns.pop_front();
        if(!conn->isConnected()) {
            invalid_conns.push_back(conn);
            continue;
        }
        if((conn->m_createTime + m_maxAliveTime) > now_ms) {
            invalid_conns.push_back(conn);
            continue;
        }
        ptr = conn;
        break;
    }
    lock.unlock();
    for(auto i : invalid_conns) {
        delete i;
    }
    m_total -= invalid_conns.size();

    if(!ptr) {
        IPAddress::ptr addr = Address::LookupAnyIPAddress(m_host);
        if(!addr) {
            DHT_LOG_ERROR(g_logger) << "get addr fail: " << m_host;
            return nullptr;
        }
        addr->setPort(m_port);
        Socket::ptr sock = m_isHttps ? SSLSocket::CreateTCP(addr) : Socket::CreateTCP(addr);
        if(!sock) {
            DHT_LOG_ERROR(g_logger) << "create sock fail: " << *addr;
            return nullptr;
        }
        if(!sock->connect(addr)) {
            DHT_LOG_ERROR(g_logger) << "sock connect fail: " << *addr;
            return nullptr;
        }

        ptr = new HttpConnection(sock);
        ++m_total;
    }
    return HttpConnection::ptr(ptr, std::bind(&HttpConnectionPool::ReleasePtr
            , std::placeholders::_1, this));
}

void HttpConnectionPool::ReleasePtr(HttpConnection* ptr, HttpConnectionPool* pool) {
    ++ptr->m_request;
    if(!ptr->isConnected()
       || ((ptr->m_createTime + pool->m_maxAliveTime) >= dht::GetCurrentMS())
       || (ptr->m_request >= pool->m_maxRequest)) {
        delete ptr;
        --pool->m_total;
        return;
    }
    MutexType::Lock lock(pool->m_mutex);
    pool->m_conns.push_back(ptr);
}

HttpResult::ptr HttpConnectionPool::doGet(const std::string& url
        , uint64_t timeout_ms
        , const std::map<std::string, std::string>& headers
        , const std::string& body) {
    return doRequest(HttpMethod::GET, url, timeout_ms, headers, body);
}

HttpResult::ptr HttpConnectionPool::doGet(Uri::ptr uri
        , uint64_t timeout_ms
        , const std::map<std::string, std::string>& headers
        , const std::string& body) {
    std::stringstream ss;
    ss << uri->getPath()
       << (uri->getQuery().empty() ? "" : "?")
       << uri->getQuery()
       << (uri->getFragment().empty() ? "" : "#")
       << uri->getFragment();
    return doGet(ss.str(), timeout_ms, headers, body);
}

HttpResult::ptr HttpConnectionPool::doPost(const std::string& url
        , uint64_t timeout_ms
        , const std::map<std::string, std::string>& headers
        , const std::string& body) {
    return doRequest(HttpMethod::POST, url, timeout_ms, headers, body);
}

HttpResult::ptr HttpConnectionPool::doPost(Uri::ptr uri
        , uint64_t timeout_ms
        , const std::map<std::string, std::string>& headers
        , const std::string& body) {
    std::stringstream ss;
    ss << uri->getPath()
       << (uri->getQuery().empty() ? "" : "?")
       << uri->getQuery()
       << (uri->getFragment().empty() ? "" : "#")
       << uri->getFragment();
    return doPost(ss.str(), timeout_ms, headers, body);
}

HttpResult::ptr HttpConnectionPool::doRequest(HttpMethod method
        , const std::string& url
        , uint64_t timeout_ms
        , const std::map<std::string, std::string>& headers
        , const std::string& body) {
    HttpRequest::ptr req = std::make_shared<HttpRequest>();
    req->setPath(url);
    req->setMethod(method);
    req->setClose(false);
    bool has_host = false;
    for(auto& i : headers) {
        if(strcasecmp(i.first.c_str(), "connection") == 0) {
            if(strcasecmp(i.second.c_str(), "keep-alive") == 0) {
                req->setClose(false);
            }
            continue;
        }

        if(!has_host && strcasecmp(i.first.c_str(), "host") == 0) {
            has_host = !i.second.empty();
        }

        req->setHeader(i.first, i.second);
    }
    if(!has_host) {
        if(m_vhost.empty()) {
            req->setHeader("Host", m_host);
        } else {
            req->setHeader("Host", m_vhost);
        }
    }
    req->setBody(body);
    return doRequest(req, timeout_ms);
}

HttpResult::ptr HttpConnectionPool::doRequest(HttpMethod method
        , Uri::ptr uri
        , uint64_t timeout_ms
        , const std::map<std::string, std::string>& headers
        , const std::string& body) {
    std::stringstream ss;
    ss << uri->getPath()
       << (uri->getQuery().empty() ? "" : "?")
       << uri->getQuery()
       << (uri->getFragment().empty() ? "" : "#")
       << uri->getFragment();
    return doRequest(method, ss.str(), timeout_ms, headers, body);
}

HttpResult::ptr HttpConnectionPool::doRequest(HttpRequest::ptr req
        , uint64_t timeout_ms) {
    auto conn = getConnection();
    if(!conn) {
        return std::make_shared<HttpResult>((int)HttpResult::Error::POOL_GET_CONNECTION
                , nullptr, "pool host:" + m_host + " port:" + std::to_string(m_port));
    }
    auto sock = conn->getSocket();
    if(!sock) {
        return std::make_shared<HttpResult>((int)HttpResult::Error::POOL_INVALID_CONNECTION
                , nullptr, "pool host:" + m_host + " port:" + std::to_string(m_port));
    }
    sock->setRecvTimeout(timeout_ms);
    int rt = conn->sendRequest(req);
    if(rt == 0) {
        return std::make_shared<HttpResult>((int)HttpResult::Error::SEND_CLOSE_BY_PEER
                , nullptr, "send request closed by peer: " + sock->getRemoteAddress()->toString());
    }
    if(rt < 0) {
        return std::make_shared<HttpResult>((int)HttpResult::Error::SEND_SOCKET_ERROR
                , nullptr, "send request socket error errno=" + std::to_string(errno)
                           + " errstr=" + std::string(strerror(errno)));
    }
    auto rsp = conn->recvResponse();
    if(!rsp) {
        return std::make_shared<HttpResult>((int)HttpResult::Error::TIMEOUT
                , nullptr, "recv response timeout: " + sock->getRemoteAddress()->toString()
                           + " timeout_ms:" + std::to_string(timeout_ms));
    }
    return std::make_shared<HttpResult>((int)HttpResult::Error::OK, rsp, "ok");
}

}
}

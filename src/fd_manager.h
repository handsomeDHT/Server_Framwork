/**
 * 文件句柄管理
 */
#ifndef SERVER_FRAMWORK_FD_MANAGER_H
#define SERVER_FRAMWORK_FD_MANAGER_H

#include <memory>
#include <vector>
#include "thread.h"
#include "iomanager.h"

namespace dht{
/**
 * 文件句柄上下文类
 * @details 管理文件句柄类型
 *         (是否socket、是否阻塞、是否管理、读/写超时时间)
 */
class FdCtx : public std::enable_shared_from_this<FdCtx> {
public:
    typedef std::shared_ptr<FdCtx> ptr;
    //通过句柄构造FdCtx
    FdCtx(int fd);
    ~FdCtx();

    bool isInit() const {return m_isInit;}
    bool isSocket() const {return m_isSocket;}
    bool isClosed() const {return m_isClosed;}
    bool closed();

    void setUserNonblock(bool v) { m_userNonblock = v;}
    bool getUserNonblock() const { return m_userNonblock; }
    void setSysNonblock(bool v) { m_sysNonblock = v; }
    bool getSysNonblock() const { return m_sysNonblock; }
    //通过判断类型type，来控制set的时间类型
    void setTimeout(int type, uint64_t v);
    uint64_t getTimeout(int type);
private:
    bool init();
private:
    bool m_isInit: 1;
    bool m_isSocket: 1;
    bool m_sysNonblock: 1;
    bool m_userNonblock: 1;
    bool m_isClosed: 1;
    int m_fd;
    //读超时时间毫秒
    uint64_t m_recvTimeout;
    //写超时时间毫秒
    uint64_t m_sendTimeout;

    //dht::IOManager* m_iomanager;
};

class FdManager{
public:
    FdManager();
    typedef RWMutex RWMutexType;
    /**
     * @brief 获取/创建文件句柄类FdCtx
     * @param[in] fd 文件句柄
     * @param[in] auto_create 是否自动创建
     * @return 返回对应文件句柄类FdCtx::ptr
     */
    FdCtx::ptr get(int fd, bool auto_create = false);
    void del(int fd);
private:
    RWMutexType m_mutex;
    std::vector<FdCtx::ptr> m_datas;
};
}
#endif //SERVER_FRAMWORK_FD_MANAGER_H
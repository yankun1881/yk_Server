#ifndef __YK__FD_MANAGER_H__
#define __YK__FD_MANAGER_H__

#include <memory>

#include <vector>
#include "thread.h"
#include "singleton.h"

namespace yk {

class FdCtx : public std::enable_shared_from_this<FdCtx> {
public:
    typedef std::shared_ptr<FdCtx> ptr;

    FdCtx(int fd);
    ~FdCtx();

    bool init();  // 初始化文件描述符上下文
    bool isInit() const { return m_isInit; }  // 判断是否已初始化
    bool isSocket() const { return m_isSocket; }  // 判断是否是socket文件

    void SetsysNonblock(bool v) { m_sysNonblock = v; }  // 设置系统级非阻塞状态
    bool GetsysNonblock() const { return m_sysNonblock; }  // 获取系统级非阻塞状态

    void SetuserNonblock(bool v) { m_userNonblock = v; }  // 设置用户主动非阻塞状态
    bool GetuserNonblock() const { return m_userNonblock; }  // 获取用户主动非阻塞状态

    bool isClose() const { return m_isClose; }  // 判断是否已关闭

    void setTimeout(int type, uint64_t v);  // 设置超时时间
    uint64_t getTimeout(int type);  // 获取超时时间

private:
    bool m_isInit: 1;  // 是否已初始化
    bool m_isSocket: 1;  // 是否是socket文件描述符
    bool m_sysNonblock: 1;  // 系统级非阻塞状态
    bool m_userNonblock: 1;  // 用户主动非阻塞状态

    bool m_isClose: 1;  // 是否已关闭
    int m_fd;  // 文件描述符
    uint64_t m_recvTimeout;  // 读超时时间
    uint64_t m_sendTimeout;  // 写超时时间
};

class FdManager {
public:
    typedef RWMutex RWMutexType;

    FdManager();

    FdCtx::ptr get(int fd, bool auto_create = false);  // 获取文件描述符上下文

    void del(int fd);  // 删除文件描述符上下文

private:
    RWMutexType m_mutex;  // 互斥量，保护数据访问
    std::vector<FdCtx::ptr> m_datas;  // 文件描述符上下文集合
};


/// 文件句柄单例
typedef Singleton<FdManager> FdMgr;

} // namespace yk

#endif

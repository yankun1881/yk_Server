#include "fdmanager.h"
#include "hook.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

namespace yk {

FdCtx::FdCtx(int fd)
    :m_isInit(false)  // 是否初始化完成
    ,m_isSocket(false)  // 是否是socket文件
    ,m_sysNonblock(false)  // 是否系统非阻塞
    ,m_userNonblock(false)  // 是否用户主动设置非阻塞
    ,m_isClose(false)  // 是否已关闭
    ,m_fd(fd)  // 文件句柄
    ,m_recvTimeout(-1)  // 读超时时间
    ,m_sendTimeout(-1)  // 写超时时间
{
    init();
}

FdCtx::~FdCtx() {
}

bool FdCtx::init() {
    if(m_isInit) {
        return true;
    }

    m_recvTimeout = -1;  // 默认读超时时间为-1，表示无限超时
    m_sendTimeout = -1;  // 默认写超时时间为-1，表示无限超时

    struct stat fd_stat;
    if(-1 == fstat(m_fd, &fd_stat)) {  // 获取文件状态信息
        m_isInit = false;  // 获取失败，表示未初始化
        m_isSocket = false;
    } else {
        m_isInit = true;  // 获取成功，表示已初始化
        m_isSocket = S_ISSOCK(fd_stat.st_mode);  // 判断是否是socket文件
    }

    if(m_isSocket) {  // 如果是socket文件
        int flags = fcntl_f(m_fd, F_GETFL, 0);  // 获取文件描述符的标志
        if(!(flags & O_NONBLOCK)) {  // 若非阻塞标志未设置
            fcntl_f(m_fd, F_SETFL, flags | O_NONBLOCK);  // 设置非阻塞标志
        }
        m_sysNonblock = true;  // 标记为系统级非阻塞
    } else {
        m_sysNonblock = false;  // 非socket文件，标记为非阻塞
    }

    m_userNonblock = false;  // 默认用户未主动设置非阻塞
    m_isClose = false;  // 默认未关闭
    return m_isInit;  // 返回初始化状态
}

void FdCtx::setTimeout(int type, uint64_t v) {
    if(type == SO_RCVTIMEO) {  // 读超时时间
        m_recvTimeout = v;
    } else {
        m_sendTimeout = v;  // 写超时时间
    }
}

uint64_t FdCtx::getTimeout(int type) {
    if(type == SO_RCVTIMEO) {
        return m_recvTimeout;
    } else {
        return m_sendTimeout;
    }
}

FdManager::FdManager() {
    m_datas.resize(64);  // 初始化文件句柄集合大小为64
}

FdCtx::ptr FdManager::get(int fd, bool auto_create) {
    if(fd == -1) {  // 文件句柄无效
        return nullptr;
    }

    RWMutexType::ReadLock lock(m_mutex);
    if((int)m_datas.size() <= fd) {  // 文件句柄超出范围
        if(auto_create == false) {  // 不自动创建
            return nullptr;
        }
    } else {
        if(m_datas[fd] || !auto_create) {  // 文件句柄已存在或不自动创建
            return m_datas[fd];
        }
    }
    lock.unlock();

    RWMutexType::WriteLock lock2(m_mutex);
    FdCtx::ptr ctx(new FdCtx(fd));  // 创建新的文件句柄上下文
    if(fd >= (int)m_datas.size()) {  // 扩展集合大小
        m_datas.resize(fd * 1.5);
    }
    m_datas[fd] = ctx;  // 将文件句柄上下文加入集合
    return ctx;
}

void FdManager::del(int fd) {
    RWMutexType::WriteLock lock(m_mutex);
    if((int)m_datas.size() <= fd) {  // 文件句柄超出范围
        return;
    }
    m_datas[fd].reset();  // 删除文件句柄上下文
}

}

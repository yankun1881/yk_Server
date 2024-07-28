#include "yk/zk_client.h"
#include "yk/log.h"

#include <semaphore.h>
namespace yk{

static yk::Logger::ptr g_logger = YK_LOG_ROOT();

void g_watcher(zhandle_t* handler, int type, int state,
    const char* path, void* wathcerCtx) {
    if (type == ZOO_SESSION_EVENT) {
        if (state == ZOO_CONNECTED_STATE) {
            sem_t* sem = (sem_t*)zoo_get_context(handler);
            sem_post(sem);
        }
    }
}

ZkClient::ZkClient()
    : zkHandler_(nullptr) {
}

ZkClient::~ZkClient() {
    if (zkHandler_ != nullptr) {
        zookeeper_close(zkHandler_);
    }
}

void ZkClient::start() {
    std::string connstr  = ip + ":" + std::to_string(port);

    /**
     * zookeeprer_mt: 多线程版本 zookeeper_st: 单线程版本
     * 有如下三个线程：
     * 1. API 调用线程
     * 2. 网络 I/O 线程(poll)
     * 3. watcher回调函数
     */
    // 初始化zk资源 这是一个异步连接函数 函数返回并不表示与zkServer连接成功
    zkHandler_ = zookeeper_init(connstr.c_str(), g_watcher, timeout, nullptr, nullptr, 0);
    // 如果zkServer返回消息 则会调用该回调函数以改变信号量唤醒当前线程
    if (nullptr == zkHandler_) {
        exit(EXIT_FAILURE);
    }

    sem_t sem;
    sem_init(&sem, 0, 0);
    zoo_set_context(zkHandler_, &sem);

    // 阻塞直到zkServer响应链接创建成功
    sem_wait(&sem);
    YK_LOG_INFO(g_logger) << "zk server connection succeededt";

}

int32_t ZkClient::create(const std::string& path, const std::string& data, int flags) {
    char path_buffer[128];
    int  bufferlen = sizeof(path_buffer);

    // 判断该节点是否存在
    int32_t flag = zoo_exists(zkHandler_, path.c_str(), 0, nullptr);
    if (flag == ZNONODE) {
        // 创建指定path的znode节点
        flag = zoo_create(zkHandler_, path.c_str(), data.c_str(), data.size(),
            &ZOO_OPEN_ACL_UNSAFE, flags, path_buffer, bufferlen);
    }
    return flag;
}

std::string ZkClient::getData(const std::string& path) {
    char buffer[64];
    int  bufferlen = sizeof(buffer);
    int  flag = zoo_get(zkHandler_, path.c_str(),0, buffer, &bufferlen, nullptr);
    if (flag != ZOK) {
        return "";
    }
    return buffer;
}

void ZkClient::getChildren(std::vector<std::string>& childrens,const std::string& path) {
    String_vector children; // 声明String_vector变量
    int rc = 0;
    // 获取子节点列表
    rc = zoo_get_children(zkHandler_, path.data(), 0, &children);

    if (rc != ZOK) {

    } else {
        // 遍历子节点列表
        for (int i = 0; i < children.count; i++) {
            childrens.push_back(children.data[i]);
        }

        // 释放子节点列表内存
        deallocate_String_vector(&children);
    }

}
 
void ZkClient::close() {
    if (zkHandler_) {
        // 关闭ZooKeeper会话
        zookeeper_close(zkHandler_);
        zkHandler_ = nullptr;
    }
    enable = false;
}

 int32_t ZkClient::exists(const std::string& path, bool b) {
    if (!zkHandler_) {
        // 如果zkHandler_为空，说明客户端未启动或已关闭
        return ZNONODE; // 或者使用其他适当的错误码
    }

    struct Stat stat;
    if (b) {
    }

    // 检查节点是否存在
    int32_t ret = zoo_exists(zkHandler_, path.c_str(), b ? true : false, &stat);
    return ret;
}



}

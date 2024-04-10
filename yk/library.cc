#include "library.h"

#include <dlfcn.h> // 使用动态链接库相关的函数
#include "yk/config.h" // 使用配置相关功能
#include "yk/env.h" // 使用环境变量相关功能
#include "yk/log.h" // 使用日志功能

namespace yk {

// 定义日志记录器
static yk::Logger::ptr g_logger = YK_LOG_NAME("system");

// 定义创建模块函数指针类型
typedef Module* (*create_module)();
// 定义销毁模块函数指针类型
typedef void (*destory_module)(Module*);

// 定义模块关闭器类
class ModuleCloser {
public:
    // 构造函数，接受模块句柄和销毁函数指针作为参数
    ModuleCloser(void* handle, destory_module d)
        :m_handle(handle)
        ,m_destory(d) {
    }

    // 重载函数调用运算符，用于销毁模块
    void operator()(Module* module) {
        // 获取模块信息
        std::string name = module->getName();
        std::string version = module->getVersion();
        std::string path = module->getFilename();
        // 销毁模块
        m_destory(module);
        // 关闭模块句柄
        int rt = dlclose(m_handle);
        // 检查关闭结果
        if(rt) {
            // 如果关闭失败，记录错误日志
            YK_LOG_ERROR(g_logger) << "dlclose handle fail handle="
                << m_handle << " name=" << name
                << " version=" << version
                << " path=" << path
                << " error=" << dlerror();
        } else {
            // 如果关闭成功，记录信息日志
            YK_LOG_INFO(g_logger) << "destory module=" << name
                << " version=" << version
                << " path=" << path
                << " handle=" << m_handle
                << " success";
        }
    }
private:
    void* m_handle; // 模块句柄
    destory_module m_destory; // 销毁函数指针
};


Module::ptr Library::GetModule(const std::string& path) {
    // 打开模块库文件
    void* handle = dlopen(path.c_str(), RTLD_NOW);
    // 检查打开结果
    if(!handle) {
        // 如果打开失败，记录错误日志
        YK_LOG_ERROR(g_logger) << "cannot load library path="
            << path << " error=" << dlerror();
        return nullptr;
    }

    // 获取创建模块函数指针
    create_module create = (create_module)dlsym(handle, "CreateModule");
    // 检查获取结果
    if(!create) {
        // 如果获取失败，记录错误日志并关闭模块句柄
        YK_LOG_ERROR(g_logger) << "cannot load symbol CreateModule in "
            << path << " error=" << dlerror();
        dlclose(handle);
        return nullptr;
    }

    // 获取销毁模块函数指针
    destory_module destory = (destory_module)dlsym(handle, "DestoryModule");
    // 检查获取结果
    if(!destory) {
        // 如果获取失败，记录错误日志并关闭模块句柄
        YK_LOG_ERROR(g_logger) << "cannot load symbol DestoryModule in "
            << path << " error=" << dlerror();
        dlclose(handle);
        return nullptr;
    }

    // 创建模块对象，并使用自定义的模块关闭器进行管理
    Module::ptr module(create(), ModuleCloser(handle, destory));
    // 设置模块文件路径
    module->setFilename(path);
    // 记录模块加载成功的信息日志
    YK_LOG_INFO(g_logger) << "load module name=" << module->getName()
        << " version=" << module->getVersion()
        << " path=" << module->getFilename()
        << " success";
    // 从配置文件夹加载配置
    Config::LoadFromConfDir(yk::EnvMgr::GetInstance()->getConfigPath(), true);
    return module;
}

}


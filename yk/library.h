#ifndef __YK_LIBRARY_H__
#define __YK_LIBRARY_H__

#include <memory>
#include "module.h"

namespace yk {

class Library {
public:
    // 获取模块的函数，接受模块路径作为参数
    static Module::ptr GetModule(const std::string& path);
};

}

#endif

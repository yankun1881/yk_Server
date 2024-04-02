#ifndef __YK_MACRO_H__
#define __YK_MACRO_H__
#include<string>
#include<assert.h>
#include"util.h"
#include"log.h"

#if defined __GNUC__ || defined __llvm__
/// LIKCLY 宏的封装, 告诉编译器优化,条件大概率成立
#   define YK_LIKELY(x)       __builtin_expect(!!(x), 1)
/// LIKCLY 宏的封装, 告诉编译器优化,条件大概率不成立
#   define YK_UNLIKELY(x)     __builtin_expect(!!(x), 0)
#else
#   define YK_LIKELY(x)      (x)
#   define YK_UNLIKELY(x)      (x)
#endif


#define YK_ASSERT1(x) \
    if(YK_UNLIKELY(!(x))) { \
        YK_LOG_ERROR(YK_LOG_ROOT()) << "ASSERTION: "#x \
            << "\nbacktrace: \n" \
            <<yk::BacktraceToString(100,2,"  "); \
        assert(x); \
    }  \

#define YK_ASSERT2(x,w) \
    if(YK_UNLIKELY(!(x))) { \
        YK_LOG_ERROR(YK_LOG_ROOT()) << "ASSERTION: "#x \
            << "\n" << w \
            << "\nbacktrace: \n" \
            <<yk::BacktraceToString(100,2,"  "); \
        assert(x); \
    }  \
        




#endif
#ifndef __YK__UTIL_H__
#define __YK__UTIL_H__

#include<pthread.h>
#include<sys/types.h>
#include<sys/syscall.h>
#include<stdint.h>
#include<vector>
#include<string>
#include <sys/time.h>

namespace yk{

pid_t GetThreadId();

uint32_t GetFiberId();

void Backtrace(std::vector<std::string>& bt, int size = 64, int skip = 1);

std::string BacktraceToString(int size = 64,int skip = 2,const std::string& prefix="");
    
//获取当前时间，单位ms
uint64_t GetCurrentMS();
uint64_t GetCurrentUS();


}
#endif
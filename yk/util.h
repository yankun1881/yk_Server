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

//时间戳转换为string
std::string TimeToStr(time_t ts = time(0), const std::string& format = "%Y-%m-%d %H:%M:%S");
//string转换为时间戳
time_t StrToTime(const char* str, const char* format = "%Y-%m-%d %H:%M:%S");

class FSUtil{
public:
    //读取该目录下所有该后缀的文件
    static void ListAllFile(std::vector<std::string>& files
                            ,const std::string& path
                            ,const std::string& subfix);
    
    //检查给定的进程 ID 文件是否指定的进程正在运行
    static bool IsRunningPidfile(const std::string& pidfile);
    static bool Mkdir(const std::string& dirname);
};

}
#endif
#ifndef __YK__UTIL_H__
#define __YK__UTIL_H__

#include<pthread.h>
#include<sys/types.h>
#include<sys/syscall.h>
#include<stdint.h>
#include<vector>
#include<string>
#include <sys/time.h>
#include <boost/lexical_cast.hpp>
#include <jsoncpp/json/json.h>

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
//到达下一天需要多少毫秒
uint64_t GetMillisUntilNextDay();

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
/**
 * 从 Map 容器中获取指定键的参数值，并尝试将其转换为指定类型 V。
 * 如果找不到指定键或转换失败，则返回默认值 def。
 * 
 * @tparam V 参数值的类型
 * @tparam Map Map 容器的类型
 * @tparam K 键的类型
 * @param m 存储参数的 Map 容器
 * @param k 要获取的参数的键
 * @param def 默认值，在找不到指定键或转换失败时返回
 * @return 参数值，类型为 V
 */
template<class V, class Map, class K>
V GetParamValue(const Map& m, const K& k, const V& def = V()) {
    auto it = m.find(k);
    if(it == m.end()) {
        return def;
    }
    try {
        return boost::lexical_cast<V>(it->second);
    } catch (...) {
    }
    return def;
}
/**
 * 从 Map 容器中获取指定键的参数值，并尝试将其转换为指定类型 V。
 * 如果找不到指定键或转换失败，则返回 false，参数值通过引用参数 v 返回。
 * 
 * @tparam V 参数值的类型
 * @tparam Map Map 容器的类型
 * @tparam K 键的类型
 * @param m 存储参数的 Map 容器
 * @param k 要获取的参数的键
 * @param v 用于存储参数值的引用，如果转换成功则存储参数值
 * @return 如果找到指定键并成功转换，则返回 true；否则返回 false
 */
template<class V, class Map, class K>
bool CheckGetParamValue(const Map& m, const K& k, V& v) {
    auto it = m.find(k);
    if(it == m.end()) {
        return false;
    }
    try {
        v = boost::lexical_cast<V>(it->second);
        return true;
    } catch (...) {
    }
    return false;
}

//base64encode
std::string base64encode(const std::string& data);
std::string base64encode(const void* data, size_t len);

std::string random_string(int length);

std::string sha1sum(const std::string& file_path);

std::string sha1sum(const void *data, size_t len);






}
#endif
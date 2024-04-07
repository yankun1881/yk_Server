
#include"util.h"
#include"log.h"
#include"fiber.h"


#include<cstring>
#include<pthread.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/syscall.h>
#include<dirent.h>
#include<execinfo.h>
#include <signal.h>
#include <sys/stat.h>

namespace yk{
static yk::Logger::ptr g_logger = YK_LOG_NAME("system");

pid_t GetThreadId(){
    return syscall(SYS_gettid); 
}
uint32_t GetFiberId(){
    return yk::Fiber::GetFiberId();
}

void Backtrace(std::vector<std::string>& bt, int size, int skip){
    void** array = (void**)malloc((sizeof(void*) * size));
    size_t s = ::backtrace(array,size);
    char** strings = backtrace_symbols(array,s);
    if(strings == nullptr){
        YK_LOG_ERROR(g_logger) << "backtrace_synbols";
        return;
    }
    for(size_t i = skip; i < s; ++i){
        bt.push_back(strings[i]);
    }
    free(strings);
    free(array);
    
}
std::string BacktraceToString(int size,int skip,const std::string& prefix){
    std::vector<std::string> bt;
    Backtrace(bt,size,skip);
    std::stringstream ss;
    for(size_t i = 0; i < bt.size(); ++i){
        ss << bt[i] << std::endl;
    }
    return ss.str();
}

uint64_t GetCurrentMS(){
    struct timeval tv;
    gettimeofday(&tv,NULL);
    return tv.tv_sec * 1000ul + tv.tv_usec /1000;
}
uint64_t GetCurrentUS(){
    struct timeval tv;
    gettimeofday(&tv,NULL);
    return tv.tv_sec *1000* 1000ul + tv.tv_usec;
}

std::string TimeToStr(time_t ts, const std::string& format) {
    struct tm tm;
    localtime_r(&ts, &tm);
    char buf[64];
    strftime(buf, sizeof(buf), format.c_str(), &tm);
    return buf;
}

time_t StrToTime(const char* str, const char* format) {
    struct tm t;
    memset(&t, 0, sizeof(t));
    if(!strptime(str, format, &t)) {
        return 0;
    }
    return mktime(&t);
}

void FSUtil::ListAllFile(std::vector<std::string>& files
                            ,const std::string& path
                            ,const std::string& subfix){
    if(access(path.c_str(), 0) != 0) {
        return;
    }
    DIR* dir = opendir(path.c_str());
    if(dir == nullptr) {
        return;
    }
    struct dirent* dp = nullptr;
    while((dp = readdir(dir)) != nullptr) {
        if(dp->d_type == DT_DIR) {
            if(!strcmp(dp->d_name, ".")
                || !strcmp(dp->d_name, "..")) {
                continue;
            }
            ListAllFile(files, path + "/" + dp->d_name, subfix);
        } else if(dp->d_type == DT_REG) {
            std::string filename(dp->d_name);
            if(subfix.empty()) {
                files.push_back(path + "/" + filename);
            } else {
                if(filename.size() < subfix.size()) {
                    continue;
                }
                if(filename.substr(filename.length() - subfix.size()) == subfix) {
                    files.push_back(path + "/" + filename);
                }
            }
        }
    }
    closedir(dir);
}

static int __lstat(const char* file, struct stat* st = nullptr) {
    struct stat lst;
    int ret = lstat(file, &lst);
    if(st) {
        *st = lst;
    }
    return ret;
}

// 检查目录是否已经存在
static int __mkdir(const char* dirname) {
    if (access(dirname, F_OK) == 0) {
        return 0; // 目录已经存在，返回成功
    }
    // 创建目录，并设置权限为用户可读、写、执行，组可读、写、执行，其他用户可读、执行
    return mkdir(dirname, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
}


bool FSUtil::IsRunningPidfile(const std::string& pidfile) {
    if(__lstat(pidfile.c_str()) != 0) {
        return false;
    }
    std::ifstream ifs(pidfile);
    std::string line;
    if(!ifs || !std::getline(ifs, line)) {
        return false;
    }
    if(line.empty()) {
        return false;
    }
    pid_t pid = atoi(line.c_str());
    if(pid <= 1) {
        return false;
    }
    if(kill(pid, 0) != 0) {
        return false;
    }
    return true;
}

bool FSUtil::Mkdir(const std::string& dirname) {
    if(__lstat(dirname.c_str()) == 0) {
        return true;
    }
    char* path = strdup(dirname.c_str());
    char* ptr = strchr(path + 1, '/');
    do {
        // 逐级创建目录
        for (; ptr; *ptr = '/', ptr = strchr(ptr + 1, '/')) {
            *ptr = '\0'; // 将下一个目录分隔符替换为字符串结束符
            if (__mkdir(path) != 0) {
                break; // 创建失败，跳出循环
            }
        }
        // 如果目录创建失败或者遇到错误，返回 false
        if (ptr != nullptr || __mkdir(path) != 0) {
            break;
        }
        // 所有目录创建成功，释放内存并返回 true
        free(path);
        return true;
    } while (0);
    free(path);
    return false;
}

}



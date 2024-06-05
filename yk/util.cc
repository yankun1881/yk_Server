
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
#include <openssl/sha.h>
#include <iomanip>
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

uint64_t GetMillisUntilNextDay() {
    // 获取当前时间的时间戳（秒）
    time_t now = time(NULL);
    
    // 将时间戳转换为tm结构体
    struct tm *tm_now = localtime(&now);
    
    // 复制当前时间结构体，以便我们可以修改它
    struct tm next_day = *tm_now;
    
    // 将日期增加一天
    next_day.tm_mday += 1;
    
    // 正常化时间结构体，以处理月份和年份的溢出
    // mktime函数会处理这种情况，例如，如果当前是2月28日，增加一天后应该是3月1日
    time_t next_day_time = mktime(&next_day);
    
    // 计算当前时间到下一天的时间戳差（秒）
    time_t seconds_until_next_day = difftime(next_day_time, now);
    
    // 将秒转换为毫秒
    uint64_t milliseconds_until_next_day = static_cast<uint64_t>(seconds_until_next_day) * 1000;
    
    return milliseconds_until_next_day;
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


std::string base64encode(const std::string& data) {
    return base64encode(data.c_str(), data.size());
}

std::string base64encode(const void* data, size_t len) {
    const char* base64 =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    std::string ret;
    ret.reserve(len * 4 / 3 + 2);

    const unsigned char* ptr = (const unsigned char*)data;
    const unsigned char* end = ptr + len;

    while(ptr < end) {
        unsigned int packed = 0;
        int i = 0;
        int padding = 0;
        for(; i < 3 && ptr < end; ++i, ++ptr) {
            packed = (packed << 8) | *ptr;
        }
        if(i == 2) {
            padding = 1;
        } else if (i == 1) {
            padding = 2;
        }
        for(; i < 3; ++i) {
            packed <<= 8;
        }

        ret.append(1, base64[packed >> 18]);
        ret.append(1, base64[(packed >> 12) & 0x3f]);
        if(padding != 2) {
            ret.append(1, base64[(packed >> 6) & 0x3f]);
        }
        if(padding == 0) {
            ret.append(1, base64[packed & 0x3f]);
        }
        ret.append(padding, '=');
    }

    return ret;
}


std::string random_string(int length) {
    const std::string charset = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    std::string result;
    result.reserve(length);
    srand(time(0)); // 使用当前时间作为随机数种子

    for (int i = 0; i < length; ++i) {
        result += charset[rand() % charset.length()];
    }

    return result;
}



std::string sha1sum(const std::string& data) {
    return sha1sum(data.c_str(), data.size());
}

std::string sha1sum(const void *data, size_t len) {
    SHA_CTX ctx;
    SHA1_Init(&ctx);
    SHA1_Update(&ctx, data, len);
    std::string result;
    result.resize(SHA_DIGEST_LENGTH);
    SHA1_Final((unsigned char*)&result[0], &ctx);
    return result;
}

}



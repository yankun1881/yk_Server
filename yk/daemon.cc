#include "daemon.h"
#include "yk/log.h"
#include "yk/config.h"
#include "yk/util.h"
#include <time.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

namespace yk {

static yk::Logger::ptr g_logger = YK_LOG_NAME("system");
static yk::ConfigVar<uint32_t>::ptr g_daemon_restart_interval //等待进程杀死 重启时间配置
    = yk::Config::Lookup("daemon.restart_interval", (uint32_t)5, "daemon restart interval");

std::string ProcessInfo::toString() const {
    std::stringstream ss;
    ss << "[ProcessInfo parent_id=" << parent_id
       << " main_id=" << main_id
       << " parent_start_time=" << yk::TimeToStr(parent_start_time)
       << " main_start_time=" << yk::TimeToStr(main_start_time)
       << " restart_count=" << restart_count << "]";
    return ss.str();
}

static int real_start(int argc, char** argv,
                     std::function<int(int argc, char** argv)> main_cb) {
    ProcessInfoMgr::GetInstance()->main_id = getpid();
    ProcessInfoMgr::GetInstance()->main_start_time = time(0);
    return main_cb(argc, argv);
}

static int real_daemon(int argc, char** argv,
                     std::function<int(int argc, char** argv)> main_cb) {
    /*if(daemon(1, 0) == -1){   //这里是直接创建子进程，应该是linux版本不同
        YK_LOG_INFO(g_logger) << "daemo errno ";
    }*/
    YK_LOG_INFO(g_logger) << "daemo errno ";
    ProcessInfoMgr::GetInstance()->parent_id = getpid();
    ProcessInfoMgr::GetInstance()->parent_start_time = time(0);
    while(true) {
        pid_t pid = fork();
        YK_LOG_INFO(g_logger) << "fork's  pid = " << pid;
        if(pid == 0) {
            //子进程返回
            ProcessInfoMgr::GetInstance()->main_id = getpid();
            ProcessInfoMgr::GetInstance()->main_start_time  = time(0);
            YK_LOG_INFO(g_logger) << "process start pid=" << getpid();
            return real_start(argc, argv, main_cb);
        } else if(pid < 0) {
            YK_LOG_ERROR(g_logger) << "fork fail return=" << pid
                << " errno=" << errno << " errstr=" << strerror(errno);
            return -1;
        } else {
            //父进程返回
            int status = 0;
            waitpid(pid, &status, 0);
            if(status) {
                if(status == 9) {
                    YK_LOG_INFO(g_logger) << "killed";
                    break;
                } else {
                    YK_LOG_ERROR(g_logger) << "child crash pid=" << pid
                        << " status=" << status;
                }
            } else {
                YK_LOG_INFO(g_logger) << "child finished pid=" << pid;
                break;
            }
            ProcessInfoMgr::GetInstance()->restart_count += 1;
            sleep(g_daemon_restart_interval->getValue());
        }
    }
    return 0;
}

int start_daemon(int argc, char** argv
                 , std::function<int(int argc, char** argv)> main_cb
                 , bool is_daemon) {
    if(!is_daemon) {
        ProcessInfoMgr::GetInstance()->parent_id = getpid();
        ProcessInfoMgr::GetInstance()->parent_start_time = time(0);
        return real_start(argc, argv, main_cb);
    }
    return real_daemon(argc, argv, main_cb);
}

}

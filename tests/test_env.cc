#include "yk/env.h"
#include <unistd.h>
#include <iostream>
#include <fstream>



int main(int argc, char** argv) {
    std::cout << "argc=" << argc << std::endl;
    yk::EnvMgr::GetInstance()->addHelp("s", "start with the terminal");
    yk::EnvMgr::GetInstance()->addHelp("d", "run as daemon");
    yk::EnvMgr::GetInstance()->addHelp("p", "print help");
    if(!yk::EnvMgr::GetInstance()->init(argc, argv)) {
        yk::EnvMgr::GetInstance()->printHelp();
        return 0;
    }

    std::cout << "exe=" << yk::EnvMgr::GetInstance()->getExe() << std::endl;
    std::cout << "cwd=" << yk::EnvMgr::GetInstance()->getCwd() << std::endl;

    std::cout << "path=" << yk::EnvMgr::GetInstance()->getEnv("PATH", "xxx") << std::endl;
    std::cout << "test=" << yk::EnvMgr::GetInstance()->getEnv("TEST", "") << std::endl;
    std::cout << "set env " << yk::EnvMgr::GetInstance()->setEnv("TEST", "yy") << std::endl;
    std::cout << "test=" << yk::EnvMgr::GetInstance()->getEnv("TEST", "") << std::endl;
    if(yk::EnvMgr::GetInstance()->has("p")) {
        yk::EnvMgr::GetInstance()->printHelp();
    }
    return 0;
}

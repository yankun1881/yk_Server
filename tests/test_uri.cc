#include "yk/uri.h"
#include <iostream>

int main(int argc, char** argv)
{
    yk::Uri::ptr uri = yk::Uri::Create("http://admin@www.baidu.com/test/uri?id=100&name=yk#frg");
    if(uri == nullptr){
        std::cout << "Create errno" <<std::endl;
        return 0;
    }
    std::cout << uri->toString() << std::endl;
    auto addr = uri->createAddress();
    std::cout << *addr << std::endl;
    return 0;
}


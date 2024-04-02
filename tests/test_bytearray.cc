#include "yk/bytearray.h"
#include "yk/log.h"
#include "yk/yk.h"

#include "yk/macro.h"

#include <string>
#include <functional>

static yk::Logger::ptr g_logger = YK_LOG_NAME("system");

template<class T>
class ts{
public:
    void test(int len, std::function<void(T)> cbw, std::function<T()> cbr, yk::ByteArray::ptr ba){
        std::vector<T> vec;
        T v;
        for(int i = 0; i < len; ++i){
            v = rand();
            YK_LOG_INFO(g_logger) <<i <<" -- " << v ;
            vec.push_back(v);
        }
        for(auto & i : vec){
            cbw(i);
        }
        ba->setPosition(0);
        for(size_t i = 0; i < vec.size(); ++i){
             v = cbr();
            YK_LOG_INFO(g_logger) <<i <<" -- " << v ;
            YK_ASSERT1(v == vec[i]);
        }
        YK_ASSERT1(ba->getReadSize() == 0);
        ba->setPosition(0);
        YK_LOG_INFO(g_logger) << "len = " << len << " size= " << ba->getSize(); 
        YK_ASSERT1(ba->writeToFile("/home/ubuntu/myServer/"+std::string(typeid(v).name())+".dat"));

        yk::ByteArray::ptr ba2(new yk::ByteArray(1));
        YK_ASSERT1(ba2->readFromFile("/home/ubuntu/myServer/"+std::string(typeid(v).name())+".dat"));
        ba2->setPosition(0);
        YK_ASSERT1(ba->toString() == ba2->toString());
        ba->clear(); 
    }   
};

int main(int argc, char** argv){
    ts<int8_t> t8;      // ostream 默认输出字符， 这里就没改了
    ts<int32_t> t32;
    ts<int64_t> t64;
    ts<uint8_t> ut8;      // ostream 默认输出字符， 这里就没改了
    ts<uint32_t> ut32;
    ts<uint64_t> ut64;
    yk::ByteArray::ptr ba(new yk::ByteArray(1));
    auto cbw = std::bind(&yk::ByteArray::writeFint8, ba, std::placeholders::_1);
    auto cbr = std::bind(&yk::ByteArray::readFint8, ba);
    t8.test(10, cbw, cbr, ba);
    auto cbw1 = std::bind(&yk::ByteArray::writeFuint8, ba, std::placeholders::_1);
    auto cbr1 = std::bind(&yk::ByteArray::readFuint8, ba);
    ut8.test(10, cbw1, cbr1, ba);
    auto cbw2 = std::bind(&yk::ByteArray::writeFint32, ba, std::placeholders::_1);
    auto cbr2 = std::bind(&yk::ByteArray::readFint32, ba);
    t32.test(10, cbw2, cbr2, ba);
    auto cbw3 = std::bind(&yk::ByteArray::writeFuint32, ba, std::placeholders::_1);
    auto cbr3 = std::bind(&yk::ByteArray::readFuint32, ba);
    ut32.test(10, cbw3, cbr3, ba);
    auto cbw4 = std::bind(&yk::ByteArray::writeFint64, ba, std::placeholders::_1);
    auto cbr4 = std::bind(&yk::ByteArray::readFint64, ba);
    t64.test(10, cbw4, cbr4, ba);
    auto cbw5 = std::bind(&yk::ByteArray::writeFuint64, ba, std::placeholders::_1);
    auto cbr5 = std::bind(&yk::ByteArray::readFuint64, ba);
    ut64.test(10, cbw5, cbr5, ba);

    return 0;
}

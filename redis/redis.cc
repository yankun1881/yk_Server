#include "redis.h"
namespace yk{

RedisConn::RedisConn(){

}
RedisConn::RedisConn(std::string _Wdress,std::string _Rdress,std::string _password)
    :Wdress(_Wdress),Rdress(_Rdress),password(_password){

}


std::shared_ptr<sw::redis::Redis> RedisConn::getMaster(){
    try {
        auto redis = std::make_shared<sw::redis::Redis>("redis://" + Wdress);
        redis->auth(password);
        return redis;
    } catch (const std::exception& e) {
        // Handle exception (logging, error reporting, etc.)
        std::cerr << "Error connecting to Redis master: " << e.what() << std::endl;
        return nullptr; // Return nullptr or throw an exception as appropriate
    }

}
std::shared_ptr<sw::redis::Redis> RedisConn::getSlave(){
    try {
        auto redis = std::make_shared<sw::redis::Redis>("redis://" + Wdress);
        redis->auth(password);
        return redis;
    } catch (const std::exception& e) {
        // Handle exception (logging, error reporting, etc.)
        std::cerr << "Error connecting to Redis master: " << e.what() << std::endl;
        return nullptr; // Return nullptr or throw an exception as appropriate
    }
}



}
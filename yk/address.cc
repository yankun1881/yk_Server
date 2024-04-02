#include"address.h"
#include<sstream>

#include<ifaddrs.h>
#include<netdb.h>
#include"endian.h"
#include"log.h"


static yk::Logger::ptr g_logger = YK_LOG_NAME("system");
namespace yk
{


template<class T>
static T CreateMask(uint32_t bits){
    return (1 << (sizeof(T)*8 - bits)) -1;
}

template<class T>
static uint32_t CountBytes(T value) {
    uint32_t result = 0;
    for(; value; ++result) {
        value &= value - 1;
    }
    return result;
}

Address::ptr Address::Create(const sockaddr* address, socklen_t addrlen){
    if(address == nullptr){
        return nullptr;
    }
    Address::ptr result;
    switch(address->sa_family){
        case AF_INET:
            result.reset(new IPv4Address(*(const sockaddr_in * )address));
            break;
        case AF_INET6:
            result.reset(new IPv6Address(*(const sockaddr_in6 * )address));
            break;
        default:
            result.reset(new UnknownAddress(*address));
            break;
    }
    return result;
}


bool Address::Lookup(std::vector<Address::ptr>& result, const std::string& host,
                     int family, int type, int protocol) {
    addrinfo hints, *results, *next;
    hints.ai_flags = 0;
    hints.ai_family = family;
    hints.ai_socktype = type;
    hints.ai_protocol = protocol;
    hints.ai_addrlen = 0;
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;
 
    std::string node;
    const char* service = NULL;

    //检查 ipv6address serivce
    if(!host.empty() && host[0] == '[') {
        const char* endipv6 = (const char*)memchr(host.c_str() + 1, ']', host.size() - 1);
        if(endipv6) {
            //TODO 
            if(*(endipv6 + 1) == ':') {
                service = endipv6 + 2;
            }
            node = host.substr(1, endipv6 - host.c_str() - 1);
        }
    }

    //检查 node serivce
    if(node.empty()) {
        service = (const char*)memchr(host.c_str(), ':', host.size());
        if(service) {
            if(!memchr(service + 1, ':', host.c_str() + host.size() - service - 1)) {
                node = host.substr(0, service - host.c_str());
                ++service;
            }
        }
    }

    if(node.empty()) {
        node = host;
    }
    int error = getaddrinfo(node.c_str(), service, &hints, &results);
    if(error) {
        YK_LOG_DEBUG(g_logger) << "Address::Lookup getaddress(" << host << ", "
            << family << ", " << type << ") err=" << error << " errstr="
            << gai_strerror(error);
        return false;
    }

    next = results;
    while(next) {
        result.push_back(Create(next->ai_addr, (socklen_t)next->ai_addrlen));
        //YK_LOG_INFO(g_logger) << ((sockaddr_in*)next->ai_addr)->sin_addr.s_addr;
        next = next->ai_next;
    }

    freeaddrinfo(results);
    return !result.empty();
}

Address::ptr Address::LookupAny(const std::string& host,
                                int family, int type, int protocol) {
    std::vector<Address::ptr> result;
    if(Lookup(result, host, family, type, protocol)) {
        return result[0];
    }
    return nullptr;
}

IPAddress::ptr Address::LookupAnyIPAddress(const std::string& host,
                                int family, int type, int protocol) {
    std::vector<Address::ptr> result;
    if(Lookup(result, host, family, type, protocol)) {
        for(auto& i : result) {
            IPAddress::ptr v = std::dynamic_pointer_cast<IPAddress>(i);
            if(v) {
                return v;
            }
        }
    }
    return nullptr;
}

bool Address::GetInterfaceAddresses(std::multimap<std::string, std::pair<Address::ptr, uint32_t>>& result, int family) {
    struct ifaddrs *next, *results;
    
    // 调用系统函数获取网络接口信息
    if(getifaddrs(&results) != 0) {
        YK_LOG_DEBUG(g_logger) << "Address::GetInterfaceAddresses getifaddrs "
            " err=" << errno << " errstr=" << strerror(errno);
        return false;
    }

    try {
        // 遍历获取到的网络接口信息
        for(next = results; next; next = next->ifa_next) {
            Address::ptr addr;
            uint32_t prefix_len = ~0u;

            // 如果指定了地址族，并且当前网络接口地址族不符合要求，则跳过
            if(family != AF_UNSPEC && family != next->ifa_addr->sa_family) {
                continue;
            }

            // 根据不同的地址族处理对应的网络接口信息
            switch(next->ifa_addr->sa_family) {
                case AF_INET:
                    {
                        // 处理 IPv4 地址
                        addr = Create(next->ifa_addr, sizeof(sockaddr_in));
                        uint32_t netmask = ((sockaddr_in*)next->ifa_netmask)->sin_addr.s_addr;
                        prefix_len = CountBytes(netmask);
                    }
                    break;
                case AF_INET6:
                    {
                        // 处理 IPv6 地址
                        addr = Create(next->ifa_addr, sizeof(sockaddr_in6));
                        in6_addr& netmask = ((sockaddr_in6*)next->ifa_netmask)->sin6_addr;
                        prefix_len = 0;
                        for(int i = 0; i < 16; ++i) {
                            prefix_len += CountBytes(netmask.s6_addr[i]);
                        }
                    }
                    break;
                default:
                    break;
            }

            // 将获取到的网络接口地址及前缀长度插入到结果容器中
            if(addr) {
                result.insert(std::make_pair(next->ifa_name,
                            std::make_pair(addr, prefix_len)));
            }
        }
    } catch (...) {
        // 捕获异常并记录日志
        YK_LOG_ERROR(g_logger) << "Address::GetInterfaceAddresses exception";
        freeifaddrs(results);
        return false;
    }
    
    // 释放获取到的网络接口信息内存
    freeifaddrs(results);
    
    // 返回是否成功获取到了网络接口地址
    return !result.empty();
}


bool Address::GetInterfaceAddresses(std::vector<std::pair<Address::ptr, uint32_t> >&result
                    ,const std::string& iface, int family) {
    if(iface.empty() || iface == "*") {
        if(family == AF_INET || family == AF_UNSPEC) {
            result.push_back(std::make_pair(Address::ptr(new IPv4Address()), 0u));
        }
        if(family == AF_INET6 || family == AF_UNSPEC) {
            result.push_back(std::make_pair(Address::ptr(new IPv6Address()), 0u));
        }
        return true;
    }

    std::multimap<std::string
          ,std::pair<Address::ptr, uint32_t> > results;

    if(!GetInterfaceAddresses(results, family)) {
        return false;
    }

    auto its = results.equal_range(iface);
    for(; its.first != its.second; ++its.first) {
        result.push_back(its.first->second);
    }
    return !result.empty();
}


int Address::getFamily()const {
    return getAddr()->sa_family;
}

std::string Address::toString(){
    std::stringstream ss;
    insert(ss);
    return ss.str();
}


/**
 * @brief 重载小于运算符，用于比较两个地址对象的大小关系
 * 
 * @param rhs 另一个地址对象s
 * @return true 如果当前地址对象小于rhs地址对象
 * @return false 如果当前地址对象大于等于rhs地址对象
 */
bool Address::operator<(const Address& rhs) const {
    // 计算两个地址的最小长度
    socklen_t minlen = std::min(getAddrLen(), rhs.getAddrLen());
    
    // 使用memcmp函数比较地址内容
    int result = memcmp(getAddr(), rhs.getAddr(), minlen);
    
    // 根据比较结果返回大小关系
    if (result < 0) {
        return true;
    } else if (result > 0) {
        return false;
    } else if (getAddrLen() < rhs.getAddrLen()) {
        return true;
    }
    
    return false;
}



IPAddress::ptr IPAddress::Create(const char* address, uint16_t port) {
    addrinfo hints, *results;
    memset(&hints, 0, sizeof(addrinfo));

    //hints.ai_flags = AI_NUMERICHOST;
    hints.ai_family = AF_UNSPEC;


    int error = getaddrinfo(address, NULL, &hints, &results);
    if(error) {
        YK_LOG_DEBUG(g_logger) << "IPAddress::Create(" << address
            << ", " << port << ") error=" << error
            << " errno=" << errno << " errstr=" << strerror(errno);
        return nullptr;
    }

    try {
        IPAddress::ptr result = std::dynamic_pointer_cast<IPAddress>(
                Address::Create(results->ai_addr, (socklen_t)results->ai_addrlen));
        if(result) {
            result->setPort(port);
        }
        freeaddrinfo(results);
        return result;
    } catch (...) {
        freeaddrinfo(results);
        return nullptr;
    }
}


bool Address::operator == (const Address& rhs) const{
    return getAddrLen() == rhs.getAddrLen()
        && memcmp(getAddr(),rhs.getAddr(),getAddrLen()) == 0 ;

}
bool Address::operator != (const Address& rhs) const{
    return !(*this == rhs);
}

IPv4Address::ptr IPv4Address::Create(const char* address, uint32_t port ){
    IPv4Address::ptr rt(new IPv4Address);
    rt->m_addr.sin_port = byteswapOnLittleEndian(port);
    int result = inet_pton(AF_INET,address,&rt->m_addr.sin_addr);
    if(result <= 0){
        YK_LOG_DEBUG(g_logger) << "IPv4Address::Create(" << address <<","
            <<port << ") rt = " << result << " error = "
            << " errstr = " << strerror(errno);
        return nullptr;
    }
    return rt;
}


IPv4Address::IPv4Address(const sockaddr_in& address){
    m_addr = address;
}


IPv4Address::IPv4Address(uint32_t address,uint32_t  port){
    memset(&m_addr,0,sizeof(m_addr));
    m_addr.sin_family = AF_INET;
    m_addr.sin_port = byteswapOnLittleEndian(port);
}

const sockaddr* IPv4Address::getAddr() const {
    return (sockaddr*)&m_addr;
}   
const socklen_t IPv4Address::getAddrLen() const {
    return sizeof(m_addr);
}
std::ostream& IPv4Address::insert(std::ostream& os) const {
    uint32_t addr = byteswapOnLittleEndian(m_addr.sin_addr.s_addr);
    os << ((addr >> 24) & 0xff) <<"."
       <<((addr >> 16)& 0xff) << "."
       <<((addr >> 8)& 0xff) << "."
       <<(addr&0xff);
    os << ":" << byteswapOnLittleEndian(m_addr.sin_port);
    return os;
}

IPAddress::ptr IPv4Address::broadcastAddress(uint32_t prefix_len) {
    if(prefix_len > 32){
        return nullptr;
    }
    sockaddr_in baddr(m_addr);

    baddr.sin_addr.s_addr |= byteswapOnLittleEndian(
        CreateMask<uint32_t> (prefix_len));
    
    return IPv4Address::ptr(new IPv4Address(baddr));
}
IPAddress::ptr IPv4Address::networkAddress(uint32_t prefix_len) {
    if(prefix_len > 32){
        return nullptr;
    }
    sockaddr_in baddr(m_addr);

    baddr.sin_addr.s_addr &= ~byteswapOnLittleEndian(
        CreateMask<uint32_t> (prefix_len));
    
    return IPv4Address::ptr(new IPv4Address(baddr));
}
IPAddress::ptr IPv4Address::subnetMask(uint32_t prefix_len) {
    sockaddr_in subnet;
    memset(&subnet,0,sizeof(subnet));
    subnet.sin_family = AF_INET;
    subnet.sin_addr.s_addr = ~byteswapOnLittleEndian(CreateMask<uint32_t>(prefix_len));
    return IPv4Address::ptr(new IPv4Address(subnet));
}

uint16_t IPv4Address::getPort() const {
    return byteswapOnLittleEndian(m_addr.sin_port);
}
void IPv4Address::setPort(uint16_t v)  {
    m_addr.sin_port = byteswapOnLittleEndian(v);
}   

IPv6Address::ptr IPv6Address::Create(const char* address,uint32_t port){
    IPv6Address::ptr rt(new IPv6Address);
    rt->m_addr.sin6_port = byteswapOnLittleEndian(port);
    int result = inet_pton(AF_INET6,address,&rt->m_addr.sin6_addr);
    if(result <= 0){
        YK_LOG_DEBUG(g_logger) << "IPv6Address::Create(" << address <<","
            <<port << ") rt = " << result << " error = "
            << " errstr = " << strerror(errno);
        return nullptr;
    }   
    return rt;
}

IPv6Address::IPv6Address(){
    memset(&m_addr,0,sizeof(m_addr));
    m_addr.sin6_family = AF_INET;
}


IPv6Address::IPv6Address(const sockaddr_in6& addr){
    m_addr = addr;
}

IPv6Address::IPv6Address(const uint8_t address[16],uint32_t  port){
    memset(&m_addr,0,sizeof(m_addr));
    m_addr.sin6_family = AF_INET;
    m_addr.sin6_port = byteswapOnLittleEndian(port);
    memcpy(&m_addr.sin6_addr.s6_addr,&address,16);
}

const sockaddr* IPv6Address::getAddr() const {
    return (sockaddr*)&m_addr;
}   
const socklen_t IPv6Address::getAddrLen() const {
    return sizeof(m_addr);
}
std::ostream& IPv6Address::insert(std::ostream& os) const {
    os << "[";
    uint16_t * addr = (uint16_t*) m_addr.sin6_addr.s6_addr;
    bool used_zeros = false;
    for(size_t i = 0; i < 8; ++i){
        if(addr[i] == 0 && !used_zeros ){
            continue;
        }
        if(i && addr[i-1] == 0 && !used_zeros){
            os << ":";
            used_zeros = true;
        }
        if(i){
            os << ":";
        }
        //hex 16进制输出数字
        os << std::hex << (int)byteswapOnLittleEndian(addr[i]) << std::dec;
    }
    if(!used_zeros && addr[7] == 0){
        os << "::";
    }
    os << "] :"<< byteswapOnLittleEndian(m_addr.sin6_port);
    return os;

}

IPAddress::ptr IPv6Address::broadcastAddress(uint32_t prefix_len) {
    sockaddr_in6 baddr(m_addr);
    baddr.sin6_addr.s6_addr[prefix_len /8] |=
        CreateMask<uint8_t>(prefix_len % 8);
    for(int i = prefix_len/8 +1; i < 16 ; ++i){
        baddr.sin6_addr.s6_addr[i] = 0xff;
    }
    return IPv6Address::ptr(new IPv6Address(baddr));
}

IPAddress::ptr IPv6Address::networkAddress(uint32_t prefix_len) {
    sockaddr_in6 baddr(m_addr);
    baddr.sin6_addr.s6_addr[prefix_len /8] &=
        CreateMask<uint8_t>(prefix_len % 8);
     
    return IPv6Address::ptr(new IPv6Address(baddr));

}
IPAddress::ptr IPv6Address::subnetMask(uint32_t prefix_len) {
    sockaddr_in6 subnet;
    memset(&subnet,0,sizeof(subnet));
    subnet.sin6_family = AF_INET6;
    subnet.sin6_addr.s6_addr[prefix_len/8] = 
        ~CreateMask<uint8_t>(prefix_len %8);

    for(int i = 0; i < (int)prefix_len /8; ++i){
        subnet.sin6_addr.s6_addr[i] = 0xff;
    }

    return IPv6Address::ptr(new IPv6Address(subnet));
}

uint16_t IPv6Address::getPort() const {
    return byteswapOnLittleEndian(m_addr.sin6_port);
}
void IPv6Address::setPort(uint16_t v)  {
    m_addr.sin6_port = byteswapOnLittleEndian(v);
}

static const size_t MAX_PATH_LEN = (sizeof((sockaddr_un*)0 ))-1;

UnixAddress::UnixAddress(){
    memset(&m_addr,0,sizeof(m_addr));
    m_addr.sun_family = AF_UNIX;
    m_length = offsetof(sockaddr_un, sun_path)+MAX_PATH_LEN;
}

UnixAddress::UnixAddress(const std::string& path){
    memset(&m_addr,0,sizeof(m_addr));
    m_addr.sun_family = AF_UNIX;
    m_length = path.size() +1;
    if(!path.empty() && path[0] == '\0'){
        --m_length;
    }
    if(m_length > sizeof(m_addr.sun_path)){
        throw std::logic_error("path too long");
    }
    memcpy(m_addr.sun_path,path.c_str(),m_length);
    m_length += offsetof(sockaddr_un,sun_path);
}
const sockaddr* UnixAddress::getAddr()const{
    return (sockaddr*)&m_addr;
}
const socklen_t UnixAddress::getAddrLen() const{
    return m_length;
}

std::ostream& UnixAddress::insert(std::ostream& os) const {
    if(m_length > offsetof(sockaddr_un,sun_path)
        && m_addr.sun_path[0] == '\0'){
        return os << "\\0" << std::string(m_addr.sun_path + 1)<<
                m_length - offsetof(sockaddr_un,sun_path)-1;
    }
    return os << m_addr.sun_path;
}


UnknownAddress::UnknownAddress(int family){
    memset(&m_addr,0,sizeof(m_addr));
    m_addr.sa_family = family;
}

UnknownAddress::UnknownAddress(const sockaddr& addr){
    m_addr = addr;
}

const sockaddr* UnknownAddress::getAddr() const {
    return &m_addr;
}   
const socklen_t UnknownAddress::getAddrLen() const{
    return sizeof(m_addr);
}
std::ostream& UnknownAddress::insert(std::ostream& os) const {
    os <<   "[UnknownAddress family" <<m_addr.sa_family <<"]";
    return os;
}


std::ostream&  operator<<(std::ostream& os,const Address& addr){
    addr.insert(os);
    return os; 
}



} // namespace yk

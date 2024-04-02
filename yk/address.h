#ifndef __YK__ADDRESS_H__
#define __YK__ADDRESS_H__


#include<memory>
#include<string>

#include<sys/socket.h>
#include<sys/types.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <map>
#include <vector>


namespace yk
{
class IPAddress;
class Address{
public:
    typedef std::shared_ptr<Address> ptr;
    static Address::ptr Create(const sockaddr* address,socklen_t addrlen);

    //通过host地址返回对应条件的所有Address
    static bool Lookup(std::vector<Address::ptr>& result,const std::string& host,
        int family = AF_UNSPEC, int type = 0,int protocol = 0);

    //通过host地址返回对应条件的任意Address
    static Address::ptr LookupAny(const std::string& host,
            int family = AF_INET, int type = 0, int protocol = 0);
    //通过host地址返回对应条件的任意IPAddress
    static std::shared_ptr<IPAddress> LookupAnyIPAddress(const std::string& host,
            int family = AF_INET, int type = 0, int protocol = 0);

    //返回本机所有网卡的<网卡名, 地址, 子网掩码位数>
    static bool GetInterfaceAddresses(std::multimap<std::string
                    ,std::pair<Address::ptr, uint32_t> >& result,
                    int family = AF_INET);
    //获取指定网卡的地址和子网掩码位数
    static bool GetInterfaceAddresses(std::vector<std::pair<Address::ptr, uint32_t> >&result
                    ,const std::string& iface, int family = AF_INET);

    virtual ~Address() {} 

    int getFamily()const;
    virtual const sockaddr* getAddr() const = 0;
    virtual const socklen_t getAddrLen() const = 0;

    virtual std::ostream& insert(std::ostream& os) const = 0;
    std::string toString();
    bool operator<(const Address& rhs) const;
    bool operator == (const Address& rhs) const;
    bool operator != (const Address& rhs) const;
};


class IPAddress : public Address{

public:
    typedef std::shared_ptr<IPAddress> ptr;
    static IPAddress::ptr Create(const char* address, uint16_t port = 0);

    // 获取广播地址
    virtual IPAddress::ptr broadcastAddress(uint32_t prefix_len) = 0;
    
    // 获取网络地址
    virtual IPAddress::ptr networkAddress(uint32_t prefix_len) = 0;
    
    // 获取子网掩码
    virtual IPAddress::ptr subnetMask(uint32_t prefix_len) = 0;
    
    virtual uint16_t getPort() const = 0;
    virtual void setPort(uint16_t v) = 0;

};


class IPv4Address : public IPAddress{

public:
    typedef std::shared_ptr<IPv4Address> ptr;
    static IPv4Address::ptr Create(const char* address, uint32_t port = 0);
    IPv4Address(const sockaddr_in& address);
    IPv4Address(uint32_t address = INADDR_ANY,uint32_t  port = 0);

    const sockaddr* getAddr() const override;   
    const socklen_t getAddrLen() const override;
    std::ostream& insert(std::ostream& os) const override;

    IPAddress::ptr broadcastAddress(uint32_t prefix_len) override;
    IPAddress::ptr networkAddress(uint32_t prefix_len) override;
    IPAddress::ptr subnetMask(uint32_t prefix_len) override;

    uint16_t getPort() const override;
    void setPort(uint16_t v)  override;
private:
    sockaddr_in m_addr;

};

class IPv6Address : public IPAddress{

public:
    typedef std::shared_ptr<IPv6Address> ptr;
    static IPv6Address::ptr Create(const char* address,uint32_t port = 0);
    IPv6Address();
    IPv6Address(const sockaddr_in6& addr);
    IPv6Address(const uint8_t address[16],uint32_t  port = 0);

    const sockaddr* getAddr() const override;   
    const socklen_t getAddrLen() const override;
    std::ostream& insert(std::ostream& os) const override;

    //IPv6没有广播，只有组播
    IPAddress::ptr broadcastAddress(uint32_t prefix_len) override;
    
    IPAddress::ptr networkAddress(uint32_t prefix_len) override;
    
    IPAddress::ptr subnetMask(uint32_t prefix_len) override;

    uint16_t getPort() const override;
    void setPort(uint16_t v)  override;
private:
    sockaddr_in6 m_addr;
};


class UnixAddress : public Address{
public:
    typedef std::shared_ptr<UnixAddress> ptr;
    UnixAddress(const std::string& path);
    UnixAddress();
    const sockaddr* getAddr() const override;   
    const socklen_t getAddrLen() const override;
    std::ostream& insert(std::ostream& os) const override;

private:
    sockaddr_un m_addr;
    socklen_t m_length;

};

class UnknownAddress : public Address{
public:
    typedef std::shared_ptr<UnknownAddress>ptr;
    UnknownAddress(int family = 0);
    UnknownAddress(const sockaddr& addr);
    const sockaddr* getAddr() const override;   
    const socklen_t getAddrLen() const override;
    std::ostream& insert(std::ostream& os) const override;
    
private:
    sockaddr m_addr;
};


std::ostream&  operator<<(std::ostream& os,const Address& addr);



} // namespace yk

#endif 
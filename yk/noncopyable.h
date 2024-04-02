#ifndef __YK__NONCOPYABLE_H__
#define __YK__NONCOPYABLE_H__

namespace yk
{
class Noncopyable{

public:
    Noncopyable() = default;
    Noncopyable(const Noncopyable& ) = delete;
    Noncopyable& operator= (const Noncopyable& ) = delete;

};
} // namespace yk

#endif 
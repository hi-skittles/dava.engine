#ifndef __DAVAENGINE_NONCOPYABLE_H__
#define __DAVAENGINE_NONCOPYABLE_H__

namespace DAVA
{
/*
 Class Noncopyable insures that derived classes cannot be copied or copy constructed.
*/
class Noncopyable
{
protected:
    Noncopyable()
    {
    }
    ~Noncopyable()
    {
    }

private:
    Noncopyable(const Noncopyable&);
    Noncopyable& operator=(const Noncopyable&);
};

} // namespace DAVA

#endif // __DAVAENGINE_NONCOPYABLE_H__

#ifndef __DAVAENGINE_SINGLETON_H__
#define __DAVAENGINE_SINGLETON_H__

namespace DAVA
{
// Singleton

template <typename T>
class Singleton
{
public:
    Singleton()
    {
        // TODO: Add assertion here DVASSERT(instance == 0 && "Singleton object should be initialized only once");
        if (instance == 0)
        {
            instance = static_cast<T*>(this);
        }
    }

    virtual ~Singleton()
    {
        instance = nullptr;
    }

    static T* Instance()
    {
        return instance;
    }

    void Release()
    {
        delete this;
        instance = nullptr;
    }

private:
    static T* instance;
};

template <typename T>
T* Singleton<T>::instance = nullptr;

} // namespace DAVA

#endif //__DAVAENGINE_SINGLETON_H__

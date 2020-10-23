#ifndef __DAVAENGINE_STATIC_SINGLETON_H__
#define __DAVAENGINE_STATIC_SINGLETON_H__

#include "Debug/DVAssert.h"

namespace DAVA
{
/**
	\ingroup baseobjects
	\brief Static Singleton is class to implement static singleton functionality in framework

 */
template <typename T>
class StaticSingleton
{
public:
    StaticSingleton()
    {
        alive = true;
    }

    virtual ~StaticSingleton()
    {
        alive = false;
    }

    static T* Instance()
    {
        static T instance;
        DVASSERT(alive == true);
        return &instance;
    }
    static bool alive;
};

template <typename T>
bool StaticSingleton<T>::alive = false;
};
#endif // __LOGENGINE_SINGLETON_H__

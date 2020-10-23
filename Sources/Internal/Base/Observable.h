#ifndef __DAVAENGINE_OBSERVABLE_H__
#define __DAVAENGINE_OBSERVABLE_H__

#include "Base/BaseTypes.h"

namespace DAVA
{
class Observer;
class Observable
{
public:
    void AddObserver(Observer* observer);
    void RemoveObserver(Observer* observer);
    void NotifyObservers();

    virtual ~Observable()
    {
    }

private:
    Set<Observer*> observers;
};
};

#endif //__DAVAENGINE_OBSERVABLE_H__

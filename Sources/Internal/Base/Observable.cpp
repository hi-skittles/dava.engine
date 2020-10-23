#include "Base/Observable.h"
#include "Base/Observer.h"

namespace DAVA
{
void Observable::AddObserver(Observer* observer)
{
    observers.insert(observer);
}

void Observable::RemoveObserver(Observer* observer)
{
    observers.erase(observer);
}

void Observable::NotifyObservers()
{
    Set<Observer*>::iterator end = observers.end();
    for (Set<Observer*>::iterator it = observers.begin(); it != end; ++it)
    {
        (*it)->HandleEvent(this);
    }
}
};

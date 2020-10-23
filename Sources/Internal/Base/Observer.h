#ifndef __DAVAENGINE_OBSERVER_H__
#define __DAVAENGINE_OBSERVER_H__

namespace DAVA
{
class Observable;
class Observer
{
public:
    virtual void HandleEvent(Observable* observable) = 0;
    virtual ~Observer()
    {
    }
};
};

#endif //__DAVAENGINE_OBSERVER_H__

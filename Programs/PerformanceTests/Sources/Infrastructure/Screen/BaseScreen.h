#ifndef __BASE_SCREEN_H__
#define __BASE_SCREEN_H__

#include "DAVAEngine.h"

using namespace DAVA;

class BaseScreen : public DAVA::UIScreen
{
public:
    BaseScreen();

    virtual void OnStart(){};
    virtual void OnFinish(){};
    virtual void RegisterScreen();

    virtual bool IsRegistered() const;
    virtual bool IsFinished() const;

    virtual void BeginFrame(){};
    virtual void EndFrame(){};

    static uint32 SCREEN_INDEX;

protected:
    ~BaseScreen(){};

    int32 currentScreenIndex;
};

#endif

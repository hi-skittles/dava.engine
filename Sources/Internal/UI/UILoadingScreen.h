#pragma once

#include "Base/BaseTypes.h"
#include "Base/RefPtr.h"
#include "UI/UIScreen.h"
#include "Concurrency/Thread.h"

namespace DAVA
{
class Thread;

class UILoadingScreen : public UIScreen
{
public:
    UILoadingScreen();
    ~UILoadingScreen() override;

    virtual void SetScreenToLoad(int32 screenId);

    void Update(float32 timeElapsed) override;
    void OnActive() override;
    void OnInactive() override;

protected:
    void ThreadMessage(BaseObject* obj, void* userData, void* callerData);
    RefPtr<Thread> thread;

private:
    int32 nextScreenId = -1;
};
}

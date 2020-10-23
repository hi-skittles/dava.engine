#pragma once

#include "Base/BaseTypes.h"
#include "Base/RefPtr.h"

namespace DAVA
{
class UIControlSystem;
class UIScreen;

class UIScreenManager final
{
public:
    UIScreenManager(UIControlSystem* uiControlSystem);
    ~UIScreenManager();

    DAVA_DEPRECATED(bool RegisterScreen(int32 screenId, UIScreen* screen));
    bool RegisterScreen(int32 screenId, const RefPtr<UIScreen>& screen);
    bool UnregisterScreen(int32 screenId);

    void SetFirst(int32 screenId);
    void SetScreen(int32 screenId);
    void ResetScreen();

    UIScreen* GetScreen(int32 screenId) const;
    UIScreen* GetScreen() const;
    int32 GetScreenId() const;

private:
    UIControlSystem* uiControlSystem = nullptr;
    UnorderedMap<int32, RefPtr<UIScreen>> screens;
    int32 activeScreenId = -1;
};
};

#include "UI/UIScreenManager.h"
#include "Logger/Logger.h"
#include "UI/UIControlSystem.h"
#include "UI/UIScreen.h"

namespace DAVA
{
UIScreenManager::UIScreenManager(UIControlSystem* _uiControlSystem)
    : uiControlSystem(_uiControlSystem)
{
}

UIScreenManager::~UIScreenManager()
{
    for (auto it = screens.begin(); it != screens.end(); ++it)
    {
        it->second->UnloadGroup();
    }

    screens.clear();
}

void UIScreenManager::SetFirst(int32 screenId)
{
    DVASSERT(activeScreenId == -1 && "[UIScreenManager::SetFirst] Called twice");

    SetScreen(screenId);
}

void UIScreenManager::SetScreen(int32 screenId)
{
    auto it = screens.find(screenId);
    if (it != screens.end())
    {
        activeScreenId = screenId;
        uiControlSystem->SetScreen(it->second.Get());
    }
    else
    {
        Logger::Error("[ScreenManager::SetScreen] Wrong screen id(%d).", screenId);
    }
}

void UIScreenManager::ResetScreen()
{
    activeScreenId = -1;
    uiControlSystem->Reset();
}

bool UIScreenManager::RegisterScreen(int32 screenId, UIScreen* screen)
{
    return RegisterScreen(screenId, RefPtr<UIScreen>::ConstructWithRetain(screen));
}

bool UIScreenManager::RegisterScreen(int32 screenId, const RefPtr<UIScreen>& screen)
{
    DVASSERT(screen != nullptr);
    auto it = screens.find(screenId);
    if (it != screens.end())
    {
        Logger::Error("[UIScreenManager::RegisterScreen] Screen id(%d) already registered.", screenId);
        return false;
    }
    screens[screenId] = screen;
    return true;
}

bool UIScreenManager::UnregisterScreen(int32 screenId)
{
    auto it = screens.find(screenId);
    if (it == screens.end())
    {
        Logger::Error("[UIScreenManager::UnregisterScreen] Screen id(%d) not registered.", screenId);
        return false;
    }
    it->second->UnloadGroup();
    screens.erase(it);
    return true;
}

UIScreen* UIScreenManager::GetScreen(int32 screenId) const
{
    auto it = screens.find(screenId);
    if (it != screens.end())
    {
        return it->second.Get();
    }
    return nullptr;
}

UIScreen* UIScreenManager::GetScreen() const
{
    return GetScreen(activeScreenId);
}

int32 UIScreenManager::GetScreenId() const
{
    return activeScreenId;
}
}

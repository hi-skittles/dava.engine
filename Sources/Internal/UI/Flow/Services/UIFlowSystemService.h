#pragma once

#include "Reflection/Reflection.h"
#include "UI/Flow/UIFlowService.h"

namespace DAVA
{
class Sprite;
class UIFlowStateComponent;

class UIFlowSystemService : public UIFlowService
{
    DAVA_VIRTUAL_REFLECTION(UIFlowSystemService, UIFlowService);

public:
    void ActivateState(const String& path, bool background);
    void DeactivateState(const String& path, bool background);
    void PreloadState(const String& path, bool background);
    bool IsStateLoaded(const String& path);
    bool IsStateActive(const String& path);
    bool HasTransitions();
    UIFlowStateComponent* GetCurrentSingleState();
    const Vector<UIFlowStateComponent*>& GetCurrentMultipleStates();
};
}

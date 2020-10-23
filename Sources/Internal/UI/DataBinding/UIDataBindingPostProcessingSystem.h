#pragma once

#include "Base/BaseTypes.h"
#include "UI/DataBinding/UIDataBindingSystem.h"

struct UIDataBindingTest;

namespace DAVA
{
class UIDataBindingPostProcessingSystem : public UISystem
{
    friend UIDataBindingTest;

public:
    UIDataBindingPostProcessingSystem(UIDataBindingSystem* dataBindingSystem);
    virtual ~UIDataBindingPostProcessingSystem();

protected:
    void Process(float32 elapsedTime) override;

private:
    UIDataBindingSystem* dataBindingSystem = nullptr;
};
}

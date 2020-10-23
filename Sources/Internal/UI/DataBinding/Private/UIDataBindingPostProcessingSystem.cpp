#include "UI/DataBinding/UIDataBindingPostProcessingSystem.h"

namespace DAVA
{
UIDataBindingPostProcessingSystem::UIDataBindingPostProcessingSystem(UIDataBindingSystem* dataBindingSystem_)
    : dataBindingSystem(dataBindingSystem_)
{
}

UIDataBindingPostProcessingSystem::~UIDataBindingPostProcessingSystem()
{
}

void UIDataBindingPostProcessingSystem::Process(float32 elapsedTime)
{
    dataBindingSystem->FinishProcess();
}
}

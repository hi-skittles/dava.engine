#include "UI/Flow/UIFlowController.h"
#include "Reflection/ReflectionRegistrator.h"
#include "UI/Flow/UIFlowContext.h"
#include "UI/UIControl.h"

namespace DAVA
{
DAVA_VIRTUAL_REFLECTION_IMPL(UIFlowController)
{
    ReflectionRegistrator<UIFlowController>::Begin()
    .ConstructorByPointer()
    .DestructorByPointer([](UIFlowController* c) { delete c; })
    .End();
}

UIFlowController::~UIFlowController() = default;
}

#include "UI/Flow/UIFlowService.h"
#include "Reflection/ReflectionRegistrator.h"

namespace DAVA
{
DAVA_VIRTUAL_REFLECTION_IMPL(UIFlowService)
{
    ReflectionRegistrator<UIFlowService>::Begin()
    .ConstructorByPointer()
    .DestructorByPointer([](UIFlowService* c) { delete c; })
    .End();
}
}

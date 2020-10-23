#include "UI/Script/UIScriptComponentController.h"
#include "Reflection/ReflectionRegistrator.h"
#include "UI/UIControl.h"

namespace DAVA
{
DAVA_VIRTUAL_REFLECTION_IMPL(UIScriptComponentController)
{
    ReflectionRegistrator<UIScriptComponentController>::Begin()
    .ConstructorByPointer()
    .DestructorByPointer([](UIScriptComponentController* c) { delete c; })
    .End();
}

UIScriptComponentController::~UIScriptComponentController() = default;
}

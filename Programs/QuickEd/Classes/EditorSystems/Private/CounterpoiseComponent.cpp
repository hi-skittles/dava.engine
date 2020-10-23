#include "Classes/EditorSystems/CounterpoiseComponent.h"

#include <UI/UIControl.h>
#include <Reflection/ReflectionRegistrator.h>
#include <Engine/Engine.h>
#include <Entity/ComponentManager.h>

DAVA_VIRTUAL_REFLECTION_IMPL(CounterpoiseComponent)
{
    DAVA::ReflectionRegistrator<CounterpoiseComponent>::Begin()[DAVA::M::HiddenField()]
    .ConstructorByPointer()
    .DestructorByPointer([](CounterpoiseComponent* o) { o->Release(); })
    .End();
}

CounterpoiseComponent* CounterpoiseComponent::Clone() const
{
    return new CounterpoiseComponent(*this);
}

const DAVA::Type* CounterpoiseComponent::GetType() const
{
    return DAVA::Type::Instance<CounterpoiseComponent>();
}

DAVA::int32 CounterpoiseComponent::GetRuntimeType() const
{
    static DAVA::int32 runtimeType = DAVA::GetEngineContext()->componentManager->GetRuntimeComponentId(GetType());
    return runtimeType;
}

#include "Classes/EditorSystems/MovableInEditorComponent.h"

#include <UI/UIControl.h>
#include <Reflection/ReflectionRegistrator.h>
#include <Engine/Engine.h>
#include <Entity/ComponentManager.h>

DAVA_VIRTUAL_REFLECTION_IMPL(MovableInEditorComponent)
{
    DAVA::ReflectionRegistrator<MovableInEditorComponent>::Begin()[DAVA::M::HiddenField()]
    .ConstructorByPointer()
    .DestructorByPointer([](MovableInEditorComponent* o) { o->Release(); })
    .End();
}

MovableInEditorComponent* MovableInEditorComponent::Clone() const
{
    return new MovableInEditorComponent(*this);
}

const DAVA::Type* MovableInEditorComponent::GetType() const
{
    return DAVA::Type::Instance<MovableInEditorComponent>();
}

DAVA::int32 MovableInEditorComponent::GetRuntimeType() const
{
    static DAVA::int32 runtimeType = DAVA::GetEngineContext()->componentManager->GetRuntimeComponentId(GetType());
    return runtimeType;
}

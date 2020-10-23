#pragma once

#include "Base/RefPtr.h"
#include "Functional/Function.h"
#include "Math/Vector.h"
#include "Reflection/Reflection.h"
#include "UI/Components/UIComponent.h"

namespace DAVA
{
class Entity;
class ScreenPositionComponent;

/** Component for setup synchronization params between UIControl and Entity. */
class UIEntityMarkerComponent : public UIComponent
{
    DAVA_VIRTUAL_REFLECTION(UIEntityMarkerComponent, UIComponent);
    DECLARE_UI_COMPONENT(UIEntityMarkerComponent);

public:
    UIEntityMarkerComponent();
    UIEntityMarkerComponent(const UIEntityMarkerComponent& src);
    UIEntityMarkerComponent& operator=(const UIEntityMarkerComponent&) = delete;

    UIEntityMarkerComponent* Clone() const override;

    /** Return pointer to target Entity. */
    Entity* GetTargetEntity() const;
    /** Setup target Entity with specified entity's pointer. */
    void SetTargetEntity(Entity* e);

protected:
    ~UIEntityMarkerComponent();

private:
    RefPtr<Entity> targetEntity;
};

inline Entity* UIEntityMarkerComponent::GetTargetEntity() const
{
    return targetEntity.Get();
}
}

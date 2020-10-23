#pragma once

#include "Entity/Component.h"
#include "Scene3D/Entity.h"
#include "Reflection/Reflection.h"

namespace DAVA
{
class RotationControllerComponent : public Component
{
public:
    Component* Clone(Entity* toEntity) override;

    DAVA_VIRTUAL_REFLECTION(RotationControllerComponent, Component);
};
};

#pragma once

#include "Scene3D/Entity.h"
#include "Scene3D/SceneFile/SerializationContext.h"
#include "Entity/Component.h"
#include "Render/Highlevel/Light.h"
#include "Reflection/Reflection.h"
#include "Base/BaseTypes.h"

namespace DAVA
{
class LightComponent : public Component
{
protected:
    ~LightComponent();

public:
    LightComponent(Light* _light = 0);

    Component* Clone(Entity* toEntity) override;
    void Serialize(KeyedArchive* archive, SerializationContext* serializationContext) override;
    void Deserialize(KeyedArchive* archive, SerializationContext* serializationContext) override;

    void SetLightObject(Light* _light);
    Light* GetLightObject() const;

    const bool IsDynamic();
    void SetDynamic(const bool& isDynamic);

    void SetLightType(const uint32& _type);
    void SetAmbientColor(const Color& _color);
    void SetDiffuseColor(const Color& _color);
    void SetIntensity(const float32& intensity);

    const uint32 GetLightType();
    const Color GetAmbientColor();
    const Color GetDiffuseColor();
    const float32 GetIntensity();

    const Vector3 GetPosition() const;
    const Vector3 GetDirection() const;
    void SetPosition(const Vector3& _position);
    void SetDirection(const Vector3& _direction);

private:
    Light* light;

    void NotifyRenderSystemLightChanged();

    DAVA_VIRTUAL_REFLECTION(LightComponent, Component);
};
};

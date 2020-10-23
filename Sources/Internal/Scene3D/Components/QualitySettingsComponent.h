#pragma once
#include "Base/BaseTypes.h"
#include "Base/FastName.h"
#include "Reflection/Reflection.h"

#include "Entity/Component.h"
#include "Scene3D/Entity.h"
#include "Scene3D/SceneFile/SerializationContext.h"

namespace DAVA
{
class QualitySettingsComponent : public Component
{
    friend class QualitySettingsSystem;

protected:
    virtual ~QualitySettingsComponent();

public:
    QualitySettingsComponent();

    Component* Clone(Entity* toEntity) override;
    void Serialize(KeyedArchive* archive, SerializationContext* serializationContext) override;
    void Deserialize(KeyedArchive* archive, SerializationContext* serializationContext) override;

    void SetFilterByType(bool filter);
    bool GetFilterByType() const;

    void SetModelType(const FastName& type);
    const FastName& GetModelType() const;

    void SetRequiredGroup(const FastName& group);
    const FastName& GetRequiredGroup() const;

    void SetRequiredQuality(const FastName& quality);
    const FastName& GetRequiredQuality() const;

private:
    FastName modelType;
    FastName requiredGroup;
    FastName requiredQuality;
    bool filterByType;

    DAVA_VIRTUAL_REFLECTION(QualitySettingsComponent, Component);
};
}

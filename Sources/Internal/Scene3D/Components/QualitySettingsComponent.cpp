#include "Scene3D/Components/QualitySettingsComponent.h"
#include "Scene3D/Systems/QualitySettingsSystem.h"
#include "Reflection/ReflectionRegistrator.h"
#include "Reflection/ReflectedMeta.h"

namespace DAVA
{
DAVA_VIRTUAL_REFLECTION_IMPL(QualitySettingsComponent)
{
    ReflectionRegistrator<QualitySettingsComponent>::Begin()
    .ConstructorByPointer()
    .Field("filterByType", &QualitySettingsComponent::GetFilterByType, &QualitySettingsComponent::SetFilterByType)[M::DisplayName("Filter By Type")]
    .Field("modelType", &QualitySettingsComponent::GetModelType, &QualitySettingsComponent::SetModelType)[M::DisplayName("Model Type")]
    .Field("requiredGroup", &QualitySettingsComponent::GetRequiredGroup, &QualitySettingsComponent::SetRequiredGroup)[M::DisplayName("Required Group")]
    .Field("requiredQuality", &QualitySettingsComponent::GetRequiredQuality, &QualitySettingsComponent::SetRequiredQuality)[M::DisplayName("Required Quality")]
    .End();
}

QualitySettingsComponent::QualitySettingsComponent()
    : Component()
    , filterByType(true)
{
}

QualitySettingsComponent::~QualitySettingsComponent()
{
}

Component* QualitySettingsComponent::Clone(Entity* toEntity)
{
    QualitySettingsComponent* component = new QualitySettingsComponent();
    component->SetEntity(toEntity);

    component->filterByType = filterByType;
    component->modelType = modelType;
    component->requiredGroup = requiredGroup;
    component->requiredQuality = requiredQuality;

    return component;
}

void QualitySettingsComponent::Serialize(KeyedArchive* archive, SerializationContext* serializationContext)
{
    Component::Serialize(archive, serializationContext);
    if (modelType.IsValid())
        archive->SetString("modelType", modelType.c_str());
    if (requiredGroup.IsValid())
        archive->SetString("requiredGroup", requiredGroup.c_str());
    if (requiredQuality.IsValid())
        archive->SetString("requiredQuality", requiredQuality.c_str());
    archive->SetBool("filterByType", filterByType);
}

void QualitySettingsComponent::Deserialize(KeyedArchive* archive, SerializationContext* serializationContext)
{
    if (archive->IsKeyExists("modelType"))
        modelType = FastName(archive->GetString("modelType"));
    if (archive->IsKeyExists("requiredGroup"))
        requiredGroup = FastName(archive->GetString("requiredGroup"));
    if (archive->IsKeyExists("requiredQuality"))
        requiredQuality = FastName(archive->GetString("requiredQuality"));
    filterByType = archive->GetBool("filterByType", filterByType);

    Component::Deserialize(archive, serializationContext);
}

void QualitySettingsComponent::SetFilterByType(bool filter)
{
    filterByType = filter;
    QualitySettingsSystem::Instance()->UpdateEntityVisibility(GetEntity());
}
bool QualitySettingsComponent::GetFilterByType() const
{
    return filterByType;
}

void QualitySettingsComponent::SetModelType(const FastName& type)
{
    modelType = type;
    QualitySettingsSystem::Instance()->UpdateEntityVisibility(GetEntity());
}

const FastName& QualitySettingsComponent::GetModelType() const
{
    return modelType;
}

void QualitySettingsComponent::SetRequiredGroup(const FastName& group)
{
    requiredGroup = group;
    QualitySettingsSystem::Instance()->UpdateEntityVisibility(GetEntity());
}
const FastName& QualitySettingsComponent::GetRequiredGroup() const
{
    return requiredGroup;
}

void QualitySettingsComponent::SetRequiredQuality(const FastName& quality)
{
    requiredQuality = quality;
    QualitySettingsSystem::Instance()->UpdateEntityVisibility(GetEntity());
}
const FastName& QualitySettingsComponent::GetRequiredQuality() const
{
    return requiredQuality;
}
};

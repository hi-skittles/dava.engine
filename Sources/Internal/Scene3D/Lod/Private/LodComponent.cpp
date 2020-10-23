#include "Scene3D/Lod/LodComponent.h"
#include "Scene3D/Entity.h"
#include "Scene3D/Components/ComponentHelpers.h"
#include "Render/Highlevel/RenderObject.h"
#include "Utils/StringFormat.h"
#include "Reflection/ReflectionRegistrator.h"
#include "Reflection/ReflectedMeta.h"

namespace DAVA
{
DAVA_VIRTUAL_REFLECTION_IMPL(LodComponent)
{
    ReflectionRegistrator<LodComponent>::Begin()
    .ConstructorByPointer()
    .Field("currentLod", &LodComponent::currentLod)[M::ReadOnly(), M::DisplayName("Current LOD")]
    .End();
}

const float32 LodComponent::INVALID_DISTANCE = -1.f;
const float32 LodComponent::MIN_LOD_DISTANCE = 0.f;
const float32 LodComponent::MAX_LOD_DISTANCE = 1000.f;

Component* LodComponent::Clone(Entity* toEntity)
{
    LodComponent* newLod = new LodComponent();
    newLod->SetEntity(toEntity);

    newLod->distances = distances;
    newLod->recursiveUpdate = recursiveUpdate;

    return newLod;
}

void LodComponent::Serialize(KeyedArchive* archive, SerializationContext* serializationContext)
{
    Component::Serialize(archive, serializationContext);

    if (archive)
    {
        ScopedPtr<KeyedArchive> lodDistArch(new KeyedArchive());
        for (uint32 i = 0; i < MAX_LOD_LAYERS; ++i)
        {
            lodDistArch->SetFloat(Format("distance%d", i), distances[i]);
        }
        archive->SetArchive("lc.loddist", lodDistArch);
    }
}

void LodComponent::Deserialize(KeyedArchive* archive, SerializationContext* serializationContext)
{
    if (archive)
    {
        KeyedArchive* lodDistArch = archive->GetArchive("lc.loddist");
        if (lodDistArch)
        {
            if (serializationContext->GetVersion() < LODSYSTEM2) //before lodsystem refactoring
            {
                for (uint32 i = 1; i < MAX_LOD_LAYERS; ++i)
                {
                    KeyedArchive* lodDistValuesArch = lodDistArch->GetArchive(KeyedArchive::GenKeyFromIndex(i));
                    if (lodDistValuesArch)
                    {
                        distances[i - 1] = lodDistValuesArch->GetFloat("ld.distance");
                    }
                }
                distances[MAX_LOD_LAYERS - 1] = std::numeric_limits<float32>::max();
                for (uint32 i = 1; i < MAX_LOD_LAYERS; ++i)
                {
                    if (distances[i] < distances[i - 1])
                    {
                        distances[i] = std::numeric_limits<float32>::max();
                    }
                }
            }
            else
            {
                for (uint32 i = 0; i < MAX_LOD_LAYERS; ++i)
                {
                    distances[i] = lodDistArch->GetFloat(Format("distance%d", i));
                }
            }
        }
    }

    Component::Deserialize(archive, serializationContext);
}
}

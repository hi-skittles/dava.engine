#include "Qt/Scene/Validation/SceneValidation.h"
#include "Classes/Qt/Scene/Validation/ValidationProgress.h"
#include "Qt/Scene/SceneHelper.h"
#include "Project/ProjectManagerData.h"

#include <Scene3D/Scene.h>
#include <Scene3D/Components/ComponentHelpers.h>
#include <Scene3D/Components/CustomPropertiesComponent.h>
#include <Scene3D/Components/SoundComponent.h>
#include <Scene3D/Components/ParticleEffectComponent.h>
#include <Scene3D/Systems/QualitySettingsSystem.h>
#include <Render/RenderBase.h>
#include <Render/Material/NMaterialNames.h>
#include <Render/Material/NMaterial.h>
#include <Render/TextureDescriptor.h>
#include <Render/Texture.h>
#include <Render/GPUFamilyDescriptor.h>
#include <Render/Texture.h>
#include <FileSystem/FileSystem.h>
#include <FileSystem/FilePath.h>
#include <Utils/StringFormat.h>

namespace SceneValidationDetails
{
using namespace DAVA;

void CollectEntitiesByName(const Entity* entity, MultiMap<FastName, const Entity*>& container)
{
    for (int32 i = 0; i < entity->GetChildrenCount(); ++i)
    {
        CollectEntitiesByName(entity->GetChild(i), container);
    }

    container.emplace(entity->GetName(), entity);
}

void CompareCustomProperties(const Entity* entity1, const Entity* entity2, ValidationProgress& validationProgress)
{
    DVASSERT(entity1 != nullptr);
    DVASSERT(entity2 != nullptr);

    KeyedArchive* props1 = GetCustomPropertiesArchieve(entity1);
    KeyedArchive* props2 = GetCustomPropertiesArchieve(entity2);

    bool entity1HasCustomProperties = (props1 != nullptr);
    bool entity2HasCustomProperties = (props2 != nullptr);

    if (entity1HasCustomProperties != entity2HasCustomProperties)
    {
        validationProgress.Alerted(DAVA::Format("Entity '%s' (id=%u) %s custom properties while entity '%s' (id=%u) %s",
                                                entity1->GetName().c_str(), entity1->GetID(),
                                                (entity1HasCustomProperties ? "has" : "hasn't"),
                                                entity2->GetName().c_str(), entity2->GetID(),
                                                (entity2HasCustomProperties ? "has" : "hasn't")));
    }
    else if (entity1HasCustomProperties && entity2HasCustomProperties)
    {
        static const char* CHECKED_PROPERTIES[] = { "CollisionType", "CollisionTypeCrashed", "editor.referenceToOwner", "Health", "MaterialKind" };

        for (const char* checkedProperty : CHECKED_PROPERTIES)
        {
            bool entity1HasProperty = props1->IsKeyExists(checkedProperty);
            bool entity2HasProperty = props2->IsKeyExists(checkedProperty);

            if (entity1HasProperty != entity2HasProperty)
            {
                validationProgress.Alerted(Format("Entity '%s' (id=%u) %s property '%s' while entity '%s' (id=%u) %s",
                                                  entity1->GetName().c_str(), entity1->GetID(),
                                                  (entity1HasProperty ? "has" : "hasn't"),
                                                  checkedProperty,
                                                  entity2->GetName().c_str(), entity2->GetID(),
                                                  (entity2HasProperty ? "has" : "hasn't")));
            }
            else if (entity1HasProperty && entity2HasProperty)
            {
                VariantType* entity1Value = props1->GetVariant(checkedProperty);
                VariantType* entity2Value = props2->GetVariant(checkedProperty);

                if (*entity1Value != *entity2Value)
                {
                    validationProgress.Alerted(Format("Property '%s' values are different for entity '%s' (id=%u) and entity '%s' (id=%u)",
                                                      checkedProperty,
                                                      entity1->GetName().c_str(), entity1->GetID(),
                                                      entity2->GetName().c_str(), entity2->GetID()));
                }
            }
        }
    }
}

void CompareSoundComponents(const Entity* entity1, const Entity* entity2, ValidationProgress& validationProgress)
{
    DVASSERT(entity1 != nullptr);
    DVASSERT(entity2 != nullptr);

    SoundComponent* soundComponent1 = GetSoundComponent(entity1);
    SoundComponent* soundComponent2 = GetSoundComponent(entity2);

    bool soundsAreEqual = true;

    if ((soundComponent1 == nullptr) != (soundComponent2 == nullptr))
    {
        soundsAreEqual = false;
    }
    else if (soundComponent1 != nullptr && soundComponent2 != nullptr)
    {
        if (soundComponent1->GetEventsCount() != soundComponent2->GetEventsCount())
        {
            soundsAreEqual = false;
        }
        else if (soundComponent1->GetEventsCount() > 0)
        {
            uint32 eventsCount = soundComponent1->GetEventsCount();
            for (uint32 iEvent = 0; iEvent < eventsCount; ++iEvent)
            {
                SoundEvent* soundEvent1 = soundComponent1->GetSoundEvent(iEvent);
                SoundEvent* soundEvent2 = soundComponent2->GetSoundEvent(iEvent);

                if (soundEvent1->GetEventName() != soundEvent2->GetEventName())
                {
                    soundsAreEqual = false;
                    break;
                }

                if (soundEvent1->GetMaxDistance() != soundEvent2->GetMaxDistance())
                {
                    soundsAreEqual = false;
                    break;
                }

                Vector<SoundEvent::SoundEventParameterInfo> eventParams1, eventParams2;
                soundEvent1->GetEventParametersInfo(eventParams1);
                soundEvent2->GetEventParametersInfo(eventParams2);

                if (eventParams1.size() != eventParams2.size())
                {
                    soundsAreEqual = false;
                    break;
                }

                auto paramLess = [](const SoundEvent::SoundEventParameterInfo& param1, const SoundEvent::SoundEventParameterInfo& param2)
                {
                    return param1.name < param2.name;
                };

                auto paramEqual = [](const SoundEvent::SoundEventParameterInfo& param1, const SoundEvent::SoundEventParameterInfo& param2)
                {
                    return (param1.name == param2.name
                            && param1.maxValue == param2.maxValue
                            && param1.minValue == param2.minValue);
                };

                std::sort(eventParams1.begin(), eventParams1.end(), paramLess);
                std::sort(eventParams2.begin(), eventParams2.end(), paramLess);
                auto firstDiff = std::mismatch(eventParams1.begin(), eventParams1.end(), eventParams2.begin(), paramEqual);

                bool vectorsAreDifferent = (firstDiff.first != eventParams1.end());
                if (vectorsAreDifferent)
                {
                    soundsAreEqual = false;
                    break;
                }

                for (SoundEvent::SoundEventParameterInfo& param : eventParams1)
                {
                    if (soundEvent1->GetParameterValue(FastName(param.name)) != soundEvent2->GetParameterValue(FastName(param.name)))
                    {
                        soundsAreEqual = false;
                        break;
                    }
                }

                if (!soundsAreEqual)
                    break;
            }
        }
    }

    if (!soundsAreEqual)
    {
        validationProgress.Alerted(Format("Entity '%s' (id=%u) and entity '%s' (id=%u) sound components are different",
                                          entity1->GetName().c_str(), entity1->GetID(),
                                          entity2->GetName().c_str(), entity2->GetID()));
    }
}

void CompareEffects(const Entity* entity1, const Entity* entity2, ValidationProgress& validationProgress)
{
    DVASSERT(entity1 != nullptr);
    DVASSERT(entity2 != nullptr);

    Vector<const Entity*> childEffects1;
    Vector<const Entity*> childEffects2;
    entity1->GetChildEntitiesWithComponent(childEffects1, Type::Instance<ParticleEffectComponent>(), false);
    entity2->GetChildEntitiesWithComponent(childEffects2, Type::Instance<ParticleEffectComponent>(), false);

    bool vectorsAreDifferent = childEffects1.size() != childEffects2.size();
    if (!vectorsAreDifferent)
    {
        auto paramLess = [](const Entity* entity1, const Entity* entity2)
        {
            return (entity1->GetName() < entity2->GetName());
        };

        auto paramEqual = [](const Entity* entity1, const Entity* entity2)
        {
            return (entity1->GetName() == entity2->GetName());
        };

        std::sort(childEffects1.begin(), childEffects1.end(), paramLess);
        std::sort(childEffects2.begin(), childEffects2.end(), paramLess);
        auto firstDiff = std::mismatch(childEffects1.begin(), childEffects1.end(), childEffects2.begin(), paramEqual);

        vectorsAreDifferent = (firstDiff.first != childEffects1.end());
    }

    if (vectorsAreDifferent)
    {
        validationProgress.Alerted(Format("Entity '%s' (id=%u) and entity '%s' (id=%u) have different effects",
                                          entity1->GetName().c_str(), entity1->GetID(),
                                          entity2->GetName().c_str(), entity2->GetID()));
    }
}

const MaterialTemplateInfo* GetTemplateByPath(const Vector<MaterialTemplateInfo>& materialTemplates, const FastName& materialTemplatePath)
{
    for (const MaterialTemplateInfo& templ : materialTemplates)
    {
        if (0 == templ.path.compare(materialTemplatePath.c_str()))
        {
            return &templ;
        }
    }

    return nullptr;
}

bool IsKnownMaterialQualityGroup(const FastName& materialGroup)
{
    size_t qcount = QualitySettingsSystem::Instance()->GetMaterialQualityGroupCount();
    for (size_t q = 0; q < qcount; ++q)
    {
        if (materialGroup == QualitySettingsSystem::Instance()->GetMaterialQualityGroupName(q))
        {
            return true;
        }
    }

    return false;
}

bool IsAssignableMaterialTemplate(const FastName& materialTemplatePath)
{
    return materialTemplatePath != NMaterialName::SHADOW_VOLUME;
}

int32 GetCollisionTypeID(const char* collisionTypeName)
{
    // copied from EditorConfig.yaml
    // temporary. should be replaced after implementaion of resource system or alike
    static const Vector<String>& collisionTypes = {
        "No Collision",
        "Tree",
        "Bush",
        "Fragile Proj",
        "Fragile ^Proj",
        "Falling",
        "Building",
        "Invisible Wall",
        "SpeedTree",
        "Water"
    };

    for (int32 i = 0; i < collisionTypes.size(); ++i)
    {
        if (collisionTypes[i] == collisionTypeName)
        {
            return i;
        }
    }

    return -1;
}

String MaterialPrettyName(NMaterial* material)
{
    DVASSERT(material);

    NMaterial* parent = material->GetParent();

    String prettyName = Format("'%s'", material->GetMaterialName().c_str());
    if (parent)
    {
        while (parent->GetParent())
        {
            parent = parent->GetParent();
        }
        prettyName.append(Format(" (parent '%s')", parent->GetMaterialName().c_str()));
    }

    return prettyName;
}

} // namespace SceneValidationDetails

SceneValidation::SceneValidation(ProjectManagerData* data)
    : projectManagerData(data)
{
}

void SceneValidation::ValidateMatrices(DAVA::Scene* scene, ValidationProgress& validationProgress)
{
    using namespace DAVA;

    validationProgress.Started("Validating matrices");

    DVASSERT(scene);

    Vector<Entity*> container;
    scene->GetChildEntitiesWithCondition(container, [](Entity* entity)
                                         {
                                             KeyedArchive* props = GetCustomPropertiesArchieve(entity);
                                             return (props != nullptr && props->IsKeyExists("editor.referenceToOwner"));
                                         });

    UnorderedSet<String> sourceScenes;
    for (const Entity* entity : container)
    {
        KeyedArchive* props = GetCustomPropertiesArchieve(entity);
        DVASSERT(props);

        const String pathToSourceScene = props->GetString("editor.referenceToOwner");
        sourceScenes.insert(pathToSourceScene);
    }

    for (const String& pathToSourceScene : sourceScenes)
    {
        ScopedPtr<Scene> sourceScene(new Scene);
        SceneFileV2::eError result = sourceScene->LoadScene(pathToSourceScene);
        if (result != SceneFileV2::eError::ERROR_NO_ERROR)
        {
            validationProgress.Alerted(Format("Can't load source model %s", pathToSourceScene.c_str()));
            continue;
        }

        if (sourceScene->GetChildrenCount() == 1)
        {
            const Matrix4& localMatrix = sourceScene->GetChild(0)->GetLocalTransform();
            const Matrix4& worldMatrix = sourceScene->GetChild(0)->GetWorldTransform();

            if (localMatrix != Matrix4::IDENTITY)
            {
                validationProgress.Alerted(Format("Source model '%s' local transform is not an identity matrix", pathToSourceScene.c_str()));
            }

            if (worldMatrix != Matrix4::IDENTITY)
            {
                validationProgress.Alerted(Format("Source model '%s' world transform is not an identity matrix", pathToSourceScene.c_str()));
            }
        }
        else
        {
            validationProgress.Alerted(Format("Source model '%s' should have only one child entity, has %i", pathToSourceScene.c_str(), sourceScene->GetChildrenCount()));
        }
    }

    validationProgress.Finished();
}

void SceneValidation::ValidateSameNames(DAVA::Scene* scene, ValidationProgress& validationProgress)
{
    using namespace DAVA;

    validationProgress.Started("Validating same names");

    DVASSERT(scene);

    MultiMap<FastName, const Entity*> entitiesByName;
    SceneValidationDetails::CollectEntitiesByName(scene, entitiesByName);

    MultiMap<FastName, const Entity*>::const_iterator currentIter = entitiesByName.begin();

    while (currentIter != entitiesByName.end())
    {
        const FastName& currentKey = currentIter->first;

        auto rangePair = entitiesByName.equal_range(currentKey);
        if (std::distance(rangePair.first, rangePair.second) > 1)
        {
            Logger::Debug("valdating %s", currentKey.c_str());
            for (auto rangeNextIter = std::next(rangePair.first);
                 rangeNextIter != rangePair.second;
                 ++rangeNextIter)
            {
                SceneValidationDetails::CompareCustomProperties(rangePair.first->second, rangeNextIter->second, validationProgress);
                SceneValidationDetails::CompareSoundComponents(rangePair.first->second, rangeNextIter->second, validationProgress);
                SceneValidationDetails::CompareEffects(rangePair.first->second, rangeNextIter->second, validationProgress);
            }
        }

        currentIter = rangePair.second;
    }

    validationProgress.Finished();
}

void SceneValidation::ValidateCollisionProperties(DAVA::Scene* scene, ValidationProgress& validationProgress)
{
    using namespace DAVA;

    validationProgress.Started("Validating collision types");

    DVASSERT(scene);

    int32 collisionTypeWaterId = SceneValidationDetails::GetCollisionTypeID("Water");
    int32 collisionTypeSpeedTreeId = SceneValidationDetails::GetCollisionTypeID("SpeedTree");

    Vector<Entity*> container;
    scene->GetChildEntitiesWithComponent(container, Type::Instance<CustomPropertiesComponent>());

    for (const Entity* entity : container)
    {
        KeyedArchive* props = GetCustomPropertiesArchieve(entity);
        DVASSERT(props);

        bool isWater = (props->GetInt32("CollisionType") == collisionTypeWaterId);
        bool isSpeedTree = (props->GetInt32("CollisionType") == collisionTypeSpeedTreeId);

        if (props->IsKeyExists("CollisionType")
            && !isWater
            && !isSpeedTree
            && !props->IsKeyExists("MaterialKind")
            && !props->IsKeyExists("FallType"))
        {
            validationProgress.Alerted(Format("Entity '%s' (id=%u) has 'CollisionType' property but hasn't 'MaterialKind' or 'FallType'",
                                              entity->GetName().c_str(), entity->GetID()));
        }
    }

    validationProgress.Finished();
}

void SceneValidation::ValidateTexturesRelevance(DAVA::Scene* scene, ValidationProgress& validationProgress)
{
    using namespace DAVA;

    validationProgress.Started("Validating textures relevance");

    DVASSERT(scene);

    SceneHelper::TextureCollector collector;
    SceneHelper::EnumerateSceneTextures(scene, collector);
    DAVA::TexturesMap& texturesMap = collector.GetTextures();

    for (const std::pair<DAVA::FilePath, DAVA::Texture*>& entry : texturesMap)
    {
        DAVA::TextureDescriptor* descriptor = entry.second->texDescriptor;
        if (nullptr != descriptor && DAVA::FileSystem::Instance()->Exists(descriptor->pathname))
        {
            for (DAVA::uint32 i = 0; i < DAVA::eGPUFamily::GPU_DEVICE_COUNT; ++i)
            {
                DAVA::eGPUFamily gpu = static_cast<DAVA::eGPUFamily>(i);
                if (descriptor->HasCompressionFor(gpu) && !descriptor->IsCompressedTextureActual(gpu))
                {
                    validationProgress.Alerted(DAVA::Format("Texture '%s' compression is not relevant for gpu %s",
                                                            descriptor->GetSourceTexturePathname().GetFilename().c_str(),
                                                            DAVA::GPUFamilyDescriptor::GetGPUName(gpu).c_str()));
                }
            }
        }
    }

    validationProgress.Finished();
}

void SceneValidation::ValidateMaterialsGroups(DAVA::Scene* scene, ValidationProgress& validationProgress)
{
    using namespace DAVA;

    validationProgress.Started("Validating material groups");

    DVASSERT(scene);

    Set<NMaterial*> materials;
    SceneHelper::BuildMaterialList(scene, materials);
    NMaterial* globalMaterial = scene->GetGlobalMaterial();
    if (nullptr != globalMaterial)
    {
        materials.erase(globalMaterial);
    }

    const Vector<MaterialTemplateInfo>* materialTemplates = projectManagerData->GetMaterialTemplatesInfo();
    DVASSERT(materialTemplates != nullptr);

    for (NMaterial* material : materials)
    {
        const FastName& materialGroup = material->GetQualityGroup();
        bool qualityGroupIsSet = false;

        String materialName = SceneValidationDetails::MaterialPrettyName(material);

        if (materialGroup.IsValid())
        {
            qualityGroupIsSet = SceneValidationDetails::IsKnownMaterialQualityGroup(materialGroup);
            if (!qualityGroupIsSet)
            {
                validationProgress.Alerted(Format("Material %s has unknown quality group '%s'", materialName.c_str(), materialGroup.c_str()));
            }
        }

        const FastName& materialTemplatePath = material->GetEffectiveFXName();
        if (materialTemplatePath.IsValid() && SceneValidationDetails::IsAssignableMaterialTemplate(materialTemplatePath))
        {
            const MaterialTemplateInfo* materialTemplate = SceneValidationDetails::GetTemplateByPath(*materialTemplates, materialTemplatePath);
            if (materialTemplate)
            {
                if (!materialTemplate->qualities.empty() && !qualityGroupIsSet)
                {
                    validationProgress.Alerted(Format("Group is not selected for material %s with multi-quality template assigned to it", materialName.c_str()));
                }
            }
            else
            {
                validationProgress.Alerted(Format("Material %s has unknown material template %s", materialName.c_str(), materialTemplatePath.c_str()));
            }
        }
    }

    validationProgress.Finished();
}

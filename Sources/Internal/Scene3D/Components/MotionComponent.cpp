#include "Animation/AnimationClip.h"
#include "FileSystem/YamlNode.h"
#include "FileSystem/YamlParser.h"
#include "Reflection/ReflectionRegistrator.h"
#include "Reflection/ReflectedMeta.h"
#include "Scene3D/Components/SkeletonComponent.h"
#include "Scene3D/Components/MotionComponent.h"
#include "Scene3D/Components/ComponentHelpers.h"
#include "Scene3D/Components/SingleComponents/MotionSingleComponent.h"
#include "Scene3D/Entity.h"
#include "Scene3D/Scene.h"
#include "Scene3D/SkeletonAnimation/MotionLayer.h"
#include "Scene3D/SkeletonAnimation/SimpleMotion.h"
#include "Scene3D/Systems/EventSystem.h"
#include "Scene3D/Systems/GlobalEventSystem.h"

namespace DAVA
{
REGISTER_CLASS(MotionComponent)

DAVA_VIRTUAL_REFLECTION_IMPL(MotionComponent)
{
    ReflectionRegistrator<MotionComponent>::Begin()
    .ConstructorByPointer()
    .Field("motionPath", &MotionComponent::GetDescriptorPath, &MotionComponent::SetDescriptorPath)[M::DisplayName("Motion File")]
    .Field("playbackRate", &MotionComponent::GetPlaybackRate, &MotionComponent::SetPlaybackRate)[M::DisplayName("Playback Rate"), M::Range(0.f, 1.f, 0.1f)]
    .Field("parameters", &MotionComponent::parameters)[M::DisplayName("Parameters")]
    .Field("motionLayers", &MotionComponent::motionLayers)[M::DisplayName("Motion Layers")]
    .Field("singleAnimationRepeatsCount", &MotionComponent::GetSingleAnimationRepeatsCount, &MotionComponent::SetSingleAnimationRepeatsCount)[M::DisplayName("Single animation Repeats")]
    .End();
}

//////////////////////////////////////////////////////////////////////////

MotionComponent::~MotionComponent()
{
    for (MotionLayer*& layer : motionLayers)
        SafeDelete(layer);

    SafeDelete(simpleMotion);
}

void MotionComponent::TriggerEvent(const FastName& trigger)
{
    for (MotionLayer* layer : motionLayers)
        layer->TriggerEvent(trigger);
}

void MotionComponent::SetParameter(const FastName& parameterID, float32 value)
{
    auto found = parameters.find(parameterID);
    if (found != parameters.end())
        found->second = value;
}

Component* MotionComponent::Clone(Entity* toEntity)
{
    MotionComponent* newComponent = new MotionComponent();
    newComponent->SetEntity(toEntity);
    newComponent->SetDescriptorPath(GetDescriptorPath());
    newComponent->SetPlaybackRate(GetPlaybackRate());
    return newComponent;
}

void MotionComponent::Serialize(KeyedArchive* archive, SerializationContext* serializationContext)
{
    Component::Serialize(archive, serializationContext);

    if (!descriptorPath.IsEmpty())
    {
        String configRelativePath = descriptorPath.GetRelativePathname(serializationContext->GetScenePath());
        archive->SetString("motion.filepath", configRelativePath);
    }

    archive->SetUInt32("simpleMotion.repeatsCount", simpleMotionRepeatsCount);
    archive->SetFloat("motion.playbackRate", playbackRate);
}

void MotionComponent::Deserialize(KeyedArchive* archive, SerializationContext* serializationContext)
{
    Component::Deserialize(archive, serializationContext);

    String relativePath = archive->GetString("motion.filepath");

    //////////////////////////////////////////////////////////////////////////
    //back compatibility
    if (relativePath.empty())
        relativePath = archive->GetString("motion.configPath");

    if (relativePath.empty())
        relativePath = archive->GetString("simpleMotion.animationPath");

    //////////////////////////////////////////////////////////////////////////

    if (!relativePath.empty())
        SetDescriptorPath(serializationContext->GetScenePath() + relativePath);

    simpleMotionRepeatsCount = archive->GetUInt32("simpleMotion.repeatsCount");
    playbackRate = archive->GetFloat("motion.playbackRate", 1.f);
}

uint32 MotionComponent::GetMotionLayersCount() const
{
    return uint32(motionLayers.size());
}

MotionLayer* MotionComponent::GetMotionLayer(uint32 index) const
{
    DVASSERT(index < GetMotionLayersCount());
    return motionLayers[index];
}

const FilePath& MotionComponent::GetDescriptorPath() const
{
    return descriptorPath;
}

void MotionComponent::SetDescriptorPath(const FilePath& path)
{
    descriptorPath = path;

    Entity* entity = GetEntity();
    if (entity && entity->GetScene())
    {
        entity->GetScene()->motionSingleComponent->reloadDescriptor.emplace_back(this);
    }
}

void MotionComponent::ReloadFromFile()
{
    for (MotionLayer*& layer : motionLayers)
        SafeDelete(layer);

    motionLayers.clear();
    parameters.clear();
    SafeDelete(simpleMotion);

    if (descriptorPath.IsEmpty())
        return;

    if (descriptorPath.IsEqualToExtension(".anim"))
    {
        simpleMotion = new SimpleMotion();
        simpleMotion->SetRepeatsCount(simpleMotionRepeatsCount);

        ScopedPtr<AnimationClip> clip(AnimationClip::Load(descriptorPath));
        simpleMotion->SetAnimation(clip);
    }
    else if (descriptorPath.IsEqualToExtension(".yaml"))
    {
        RefPtr<YamlParser> parser = YamlParser::Create(descriptorPath);
        if (parser != nullptr)
        {
            YamlNode* rootNode = parser->GetRootNode();
            if (rootNode)
            {
                const YamlNode* motionLayersNode = rootNode->Get("motion-layers");
                if (motionLayersNode != nullptr && motionLayersNode->GetType() == YamlNode::TYPE_ARRAY)
                {
                    uint32 motionLayersCount = motionLayersNode->GetCount();
                    for (uint32 m = 0; m < motionLayersCount; ++m)
                    {
                        const YamlNode* motionLayerNode = motionLayersNode->Get(m);
                        MotionLayer* motionLayer = MotionLayer::LoadFromYaml(motionLayerNode);
                        if (motionLayer != nullptr)
                        {
                            motionLayers.push_back(motionLayer);

                            for (const FastName& p : motionLayer->GetParameterIDs())
                                parameters[p] = 0.f;
                        }
                    }

                    for (MotionLayer* motion : motionLayers)
                    {
                        for (const FastName& p : motion->GetParameterIDs())
                            motion->BindParameter(p, &parameters[p]);
                    }
                }
            }
        }
    }
}

Vector<FilePath> MotionComponent::GetDependencies() const
{
    Vector<FilePath> result;

    if (!descriptorPath.IsEmpty())
    {
        result.push_back(descriptorPath);

        if (descriptorPath.IsEqualToExtension(".yaml"))
        {
            RefPtr<YamlParser> parser = YamlParser::Create(descriptorPath);
            if (parser != nullptr)
            {
                Set<FilePath> dependencies;
                GetDependenciesRecursive(parser->GetRootNode(), &dependencies);

                for (const FilePath& fp : dependencies)
                    result.push_back(fp);
            }
        }
    }

    return result;
}

void MotionComponent::GetDependenciesRecursive(const YamlNode* node, Set<FilePath>* dependencies) const
{
    if (node != nullptr)
    {
        if (node->GetType() == YamlNode::TYPE_MAP)
        {
            const YamlNode* clipNode = node->Get("clip");
            if (clipNode != nullptr)
                dependencies->insert(FilePath(clipNode->AsString()));
        }

        uint32 childrenCount = node->GetCount();
        for (uint32 c = 0; c < childrenCount; ++c)
            GetDependenciesRecursive(node->Get(c), dependencies);
    }
}
}

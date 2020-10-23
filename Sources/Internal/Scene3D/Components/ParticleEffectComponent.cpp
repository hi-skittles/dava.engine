#include "Scene3D/Components/ParticleEffectComponent.h"
#include "Scene3D/Components/RenderComponent.h"
#include "Scene3D/Components/TransformComponent.h"
#include "Scene3D/Entity.h"
#include "Scene3D/Components/ComponentHelpers.h"
#include "Scene3D/Lod/LodComponent.h"
#include "Scene3D/Lod/LodSystem.h"
#include "Scene3D/Systems/ParticleEffectSystem.h"
#include "Scene3D/Systems/EventSystem.h"
#include "Scene3D/Systems/GlobalEventSystem.h"
#include "Scene3D/Systems/QualitySettingsSystem.h"
#include "Reflection/ReflectionRegistrator.h"
#include "Reflection/ReflectedMeta.h"
#include <Math/Transform.h>

namespace DAVA
{
DAVA_VIRTUAL_REFLECTION_IMPL(ParticleEffectComponent)
{
    ReflectionRegistrator<ParticleEffectComponent>::Begin()[M::CantBeCreatedManualyComponent()]
    .ConstructorByPointer()
    .Field("repeatsCount", &ParticleEffectComponent::repeatsCount)[M::DisplayName("Repeats Count")]
    .Field("stopWhenEmpty", &ParticleEffectComponent::stopWhenEmpty)[M::DisplayName("Stop When Empty")]
    .Field("effectDuration", &ParticleEffectComponent::effectDuration)[M::DisplayName("Duration")]
    .Field("clearOnRestart", &ParticleEffectComponent::clearOnRestart)[M::DisplayName("Clear On Restart")]
    .Field("startFromTime", &ParticleEffectComponent::GetStartFromTime, &ParticleEffectComponent::SetStartFromTime)[M::DisplayName("Start From Time")]
    .Field("visibleReflection", &ParticleEffectComponent::GetReflectionVisible, &ParticleEffectComponent::SetReflectionVisible)[M::DisplayName("Visible Reflection")]
    .Field("visibleRefraction", &ParticleEffectComponent::GetRefractionVisible, &ParticleEffectComponent::SetRefractionVisible)[M::DisplayName("Visible Refraction")]
    .Field("clippingVisible", &ParticleEffectComponent::GetClippingVisible, &ParticleEffectComponent::SetClippingVisible)[M::DisplayName("Clipping always visible")]
    .End();
}

ParticleEffectComponent::ParticleEffectComponent()
{
    effectData.infoSources.resize(1);
    effectData.infoSources[0].size = Vector2(1, 1);

    // world transform doesn't effect particle render object drawing
    // instead particles are generated in corresponding world position
    effectRenderObject = new ParticleRenderObject(&effectData);
    effectRenderObject->SetWorldMatrixPtr(&Matrix4::IDENTITY);

    if (QualitySettingsSystem::Instance()->IsOptionEnabled(QualitySettingsSystem::QUALITY_OPTION_LOD0_EFFECTS))
    {
        desiredLodLevel = 0;
        activeLodLevel = 0;
    }
}

ParticleEffectComponent::~ParticleEffectComponent()
{
    ClearCurrentGroups();
    if (state != STATE_STOPPED)
    {
        Entity* e = GetEntity();
        if (e && e->GetScene())
        {
            e->GetScene()->particleEffectSystem->RemoveFromActive(this);
        }
    }
    SafeRelease(effectRenderObject);
    emitterInstances.clear();
}

Component* ParticleEffectComponent::Clone(Entity* toEntity)
{
    ParticleEffectComponent* newComponent = new ParticleEffectComponent();
    newComponent->SetEntity(toEntity);
    newComponent->repeatsCount = repeatsCount;
    newComponent->stopWhenEmpty = stopWhenEmpty;
    newComponent->playbackComplete = playbackComplete;
    newComponent->effectDuration = effectDuration;
    newComponent->clearOnRestart = clearOnRestart;
    newComponent->startFromTime = startFromTime;

    for (const auto& instance : emitterInstances)
    {
        newComponent->AddEmitterInstance(ScopedPtr<ParticleEmitterInstance>(instance->Clone()));
    }
    newComponent->RebuildEffectModifiables();
    newComponent->effectRenderObject->SetFlags(effectRenderObject->GetFlags());
    return newComponent;
}

void ParticleEffectComponent::SetSortingOffset(uint32 offset)
{
    effectRenderObject->SetSortingOffset(offset);
}

void ParticleEffectComponent::Start()
{
    isPaused = false;
    GlobalEventSystem::Instance()->Event(this, EventSystem::START_PARTICLE_EFFECT);
}

void ParticleEffectComponent::Stop(bool isDeleteAllParticles)
{
    if (state == STATE_STOPPED)
        return;
    if (isDeleteAllParticles)
    {
        ClearCurrentGroups();
        effectData.infoSources.resize(1);
        GlobalEventSystem::Instance()->Event(this, EventSystem::STOP_PARTICLE_EFFECT);
    }
    else
    {
        state = STATE_STOPPING;
        SetGroupsFinishing();
    }
}

void ParticleEffectComponent::Pause(bool isPaused /*= true*/)
{
    this->isPaused = isPaused;
}

bool ParticleEffectComponent::IsStopped()
{
    return state == STATE_STOPPED;
}

bool ParticleEffectComponent::IsPaused()
{
    return isPaused;
}

void ParticleEffectComponent::Step(float32 delta)
{
    GetEntity()->GetScene()->particleEffectSystem->UpdateEffect(this, delta, delta);
}

void ParticleEffectComponent::Restart(bool isDeleteAllParticles)
{
    isPaused = false;
    if (isDeleteAllParticles)
        ClearCurrentGroups();
    currRepeatsCont = 0;
    GlobalEventSystem::Instance()->Event(this, EventSystem::START_PARTICLE_EFFECT);
}

void ParticleEffectComponent::StopAfterNRepeats(int32 numberOfRepeats)
{
    repeatsCount = numberOfRepeats;
}

void ParticleEffectComponent::StopWhenEmpty(bool value /*= true*/)
{
    stopWhenEmpty = value;
}

void ParticleEffectComponent::ClearGroup(ParticleGroup& group)
{
    Particle* current = group.head;
    while (current)
    {
        Particle* next = current->next;
        delete current;
        current = next;
    }
    group.layer->Release();
    group.emitter->Release();
}

void ParticleEffectComponent::ClearCurrentGroups()
{
    for (List<ParticleGroup>::iterator it = effectData.groups.begin(), e = effectData.groups.end(); it != e; ++it)
    {
        ClearGroup(*it);
    }
    effectData.groups.clear();
}

void ParticleEffectComponent::SetRenderObjectVisible(bool visible)
{
    if (visible)
        effectRenderObject->AddFlag(RenderObject::VISIBLE);
    else
        effectRenderObject->RemoveFlag(RenderObject::VISIBLE);
}

void ParticleEffectComponent::SetGroupsFinishing()
{
    for (List<ParticleGroup>::iterator it = effectData.groups.begin(), e = effectData.groups.end(); it != e; ++it)
    {
        (*it).finishingGroup = true;
    }
}

void ParticleEffectComponent::SetPlaybackCompleteMessage(const Message& msg)
{
    playbackComplete = msg;
}

float32 ParticleEffectComponent::GetPlaybackSpeed()
{
    return playbackSpeed;
}

void ParticleEffectComponent::SetPlaybackSpeed(float32 value)
{
    playbackSpeed = value;
}

void ParticleEffectComponent::SetDesiredLodLevel(int32 level)
{
    if (QualitySettingsSystem::Instance()->IsOptionEnabled(QualitySettingsSystem::QUALITY_OPTION_LOD0_EFFECTS))
    {
        desiredLodLevel = 0;
    }
    else
    {
        desiredLodLevel = level;
    }
}

void ParticleEffectComponent::SetExtertnalValue(const String& name, float32 value)
{
    externalValues[name] = value;
    for (MultiMap<String, ModifiablePropertyLineBase *>::iterator it = externalModifiables.lower_bound(name), e = externalModifiables.upper_bound(name); it != e; ++it)
        (*it).second->SetModifier(value);
}

float32 ParticleEffectComponent::GetExternalValue(const String& name)
{
    Map<String, float32>::iterator it = externalValues.find(name);
    if (it != externalValues.end())
        return (*it).second;
    else
        return 0.0f;
}

Set<String> ParticleEffectComponent::EnumerateVariables()
{
    Set<String> res;
    for (MultiMap<String, ModifiablePropertyLineBase *>::iterator it = externalModifiables.begin(), e = externalModifiables.end(); it != e; ++it)
        res.insert((*it).first);
    return res;
}

void ParticleEffectComponent::RegisterModifiable(ModifiablePropertyLineBase* propertyLine)
{
    externalModifiables.insert(std::make_pair(propertyLine->GetValueName(), propertyLine));
    Map<String, float32>::iterator it = externalValues.find(propertyLine->GetValueName());
    if (it != externalValues.end())
        propertyLine->SetModifier((*it).second);
}
void ParticleEffectComponent::UnRegisterModifiable(ModifiablePropertyLineBase* propertyLine)
{
    for (MultiMap<String, ModifiablePropertyLineBase *>::iterator it = externalModifiables.lower_bound(propertyLine->GetValueName()), e = externalModifiables.upper_bound(propertyLine->GetValueName()); it != e; ++it)
    {
        if ((*it).second == propertyLine)
        {
            externalModifiables.erase(it);
            return;
        }
    }
}

void ParticleEffectComponent::RebuildEffectModifiables()
{
    externalModifiables.clear();
    List<ModifiablePropertyLineBase*> modifiables;
    for (auto& instance : emitterInstances)
    {
        instance->GetEmitter()->GetModifableLines(modifiables);
    }

    for (List<ModifiablePropertyLineBase *>::iterator it = modifiables.begin(), e = modifiables.end(); it != e; ++it)
    {
        externalModifiables.insert(std::make_pair((*it)->GetValueName(), (*it)));
        Map<String, float32>::iterator itName = externalValues.find((*it)->GetValueName());
        if (itName != externalValues.end())
            (*it)->SetModifier((*itName).second);
    }
}

int32 ParticleEffectComponent::GetActiveParticlesCount()
{
    int32 totalActiveParticles = 0;
    for (List<ParticleGroup>::iterator it = effectData.groups.begin(), e = effectData.groups.end(); it != e; ++it)
        totalActiveParticles += (*it).activeParticleCount;

    return totalActiveParticles;
}

void ParticleEffectComponent::Serialize(KeyedArchive* archive, SerializationContext* serializationContext)
{
    Component::Serialize(archive, serializationContext);
    archive->SetUInt32("pe.version", 1);
    archive->SetBool("pe.stopWhenEmpty", stopWhenEmpty);
    archive->SetFloat("pe.effectDuration", effectDuration);
    archive->SetUInt32("pe.repeatsCount", repeatsCount);
    archive->SetBool("pe.clearOnRestart", clearOnRestart);
    archive->SetUInt32("pe.emittersCount", static_cast<uint32>(emitterInstances.size()));
    archive->SetFloat("pe.startFromTime", startFromTime);
    KeyedArchive* emittersArch = new KeyedArchive();
    for (uint32 i = 0; i < emitterInstances.size(); ++i)
    {
        const auto& instance = emitterInstances[i];
        KeyedArchive* emitterArch = new KeyedArchive();
        String filename = instance->GetFilePath().GetRelativePathname(serializationContext->GetScenePath());
        emitterArch->SetString("emitter.filename", filename);
        emitterArch->SetVector3("emitter.position", instance->GetSpawnPosition());
        emittersArch->SetArchive(KeyedArchive::GenKeyFromIndex(i), emitterArch);
        emitterArch->Release();
    }
    archive->SetArchive("pe.emitters", emittersArch);

    archive->SetUInt32("ro.flags", effectRenderObject->GetFlags() & PARTICLE_FLAGS_SERIALIZATION_CRITERIA);
    emittersArch->Release();
}

void ParticleEffectComponent::Deserialize(KeyedArchive* archive, SerializationContext* serializationContext)
{
    Component::Deserialize(archive, serializationContext);
    loadedVersion = archive->GetUInt32("pe.version", 0);

    if (loadedVersion == 1) //new effect - load everything here
    {
        const ParticlesQualitySettings::FilepathSelector* filepathSelector = QualitySettingsSystem::Instance()->GetParticlesQualitySettings().GetOrCreateFilepathSelector();

        stopWhenEmpty = archive->GetBool("pe.stopWhenEmpty");
        effectDuration = archive->GetFloat("pe.effectDuration");
        repeatsCount = archive->GetUInt32("pe.repeatsCount");
        clearOnRestart = archive->GetBool("pe.clearOnRestart");
        uint32 emittersCount = archive->GetUInt32("pe.emittersCount");
        startFromTime = archive->GetFloat("pe.startFromTime");
        KeyedArchive* emittersArch = archive->GetArchive("pe.emitters");
        emitterInstances.resize(emittersCount);
        for (uint32 i = 0; i < emittersCount; ++i)
        {
            emitterInstances[i].ConstructInplace(this);

            KeyedArchive* emitterArch = emittersArch->GetArchive(KeyedArchive::GenKeyFromIndex(i));
            String filename = emitterArch->GetString("emitter.filename");
            if (!filename.empty())
            {
                emitterInstances[i]->SetFilePath(serializationContext->GetScenePath() + filename);
                FilePath qualityFilepath = emitterInstances[i]->GetFilePath();
                if (filepathSelector)
                {
                    qualityFilepath = filepathSelector->SelectFilepath(emitterInstances[i]->GetFilePath());
                }
                emitterInstances[i]->SetEmitter(ParticleEmitter::LoadEmitter(qualityFilepath));
            }
            else
            {
                emitterInstances[i]->SetEmitter(new ParticleEmitter());
            }
            emitterInstances[i]->SetSpawnPosition(emitterArch->GetVector3("emitter.position"));
        }
        uint32 savedFlags = RenderObject::SERIALIZATION_CRITERIA & archive->GetUInt32("ro.flags", RenderObject::VISIBLE);
        effectRenderObject->SetFlags(savedFlags | (effectRenderObject->GetFlags() & ~PARTICLE_FLAGS_SERIALIZATION_CRITERIA));
        RebuildEffectModifiables();
    }
}

void ParticleEffectComponent::CollapseOldEffect(SerializationContext* serializationContext)
{
    Entity* entity = GetEntity();
    bool lodDefined = false;
    effectDuration = 0;
    Vector3 effectScale = Vector3(1, 1, 1);
    Entity* currEntity = entity;
    while (currEntity)
    {
        TransformComponent* transform = currEntity->GetComponent<TransformComponent>();
        effectScale *= transform->GetLocalMatrix().GetScaleVector();
        currEntity = currEntity->GetParent();
    }
    for (int32 i = 0, sz = entity->GetChildrenCount(); i < sz; ++i)
    {
        Entity* child = entity->GetChild(i);
        PartilceEmitterLoadProxy* emitterProxy = NULL;
        RenderComponent* renderComponent = child->GetComponent<RenderComponent>();
        if (renderComponent)
            emitterProxy = static_cast<PartilceEmitterLoadProxy*>(renderComponent->GetRenderObject());
        if (!emitterProxy)
            continue;

        ParticleEmitterInstance* instance = new ParticleEmitterInstance(this);
        instance->SetFilePath(serializationContext->GetScenePath() + emitterProxy->emitterFilename);
        if (!emitterProxy->emitterFilename.empty())
        {
            FilePath qualityFilepath = emitterInstances[i]->GetFilePath();
            const ParticlesQualitySettings::FilepathSelector* filepathSelector = QualitySettingsSystem::Instance()->GetParticlesQualitySettings().GetOrCreateFilepathSelector();
            if (filepathSelector)
            {
                qualityFilepath = filepathSelector->SelectFilepath(qualityFilepath);
            }

            auto emitter = ParticleEmitter::LoadEmitter(qualityFilepath);
            instance->SetEmitter(emitter);
            if (effectDuration < emitter->lifeTime)
                effectDuration = emitter->lifeTime;
        }
        else
        {
            instance->SetEmitter(new ParticleEmitter());
        }
        instance->GetEmitter()->name = child->GetName();

        TransformComponent* transform = child->GetComponent<TransformComponent>();
        instance->SetSpawnPosition(transform->GetLocalTransform().GetTranslation() * effectScale);
        emitterInstances.emplace_back(instance);

        if (!lodDefined)
        {
            LodComponent* lodComponent = child->GetComponent<LodComponent>();
            if (lodComponent)
            {
                entity->AddComponent(lodComponent->Clone(entity));
                lodDefined = true;
            }
        }
    }

    entity->RemoveAllChildren();
    RebuildEffectModifiables();
}

uint32 ParticleEffectComponent::GetEmittersCount() const
{
    return static_cast<uint32>(emitterInstances.size());
}

Vector3 ParticleEffectComponent::GetSpawnPosition(int32 id) const
{
    DVASSERT((id >= 0) && (id < static_cast<int32>(emitterInstances.size())));
    return emitterInstances[id]->GetSpawnPosition();
}

void ParticleEffectComponent::SetSpawnPosition(int32 id, const Vector3& position)
{
    DVASSERT((id >= 0) && (id < static_cast<int32>(emitterInstances.size())));
    emitterInstances[id]->SetSpawnPosition(position);
}

DAVA::FilePath ParticleEffectComponent::GetOriginalConfigPath(int32 id) const
{
    DVASSERT((id >= 0) && (id < static_cast<int32>(emitterInstances.size())));
    return emitterInstances[id]->GetFilePath();
}

void ParticleEffectComponent::SetOriginalConfigPath(int32 id, const FilePath& filepath)
{
    DVASSERT((id >= 0) && (id < static_cast<int32>(emitterInstances.size())));
    emitterInstances[id]->SetFilePath(filepath);
}

ParticleEmitterInstance* ParticleEffectComponent::GetEmitterInstance(uint32 id) const
{
    DVASSERT(id < emitterInstances.size());
    return emitterInstances[id].Get();
}

ParticleEmitter* ParticleEffectComponent::GetEmitter(uint32 id) const
{
    return GetEmitterInstance(id)->GetEmitter();
}

void ParticleEffectComponent::AddEmitterInstance(ParticleEmitter* emitter)
{
    emitterInstances.emplace_back(new ParticleEmitterInstance(this, emitter));
}

void ParticleEffectComponent::AddEmitterInstance(ParticleEmitterInstance* instance)
{
    instance->SetOwner(this);
    emitterInstances.emplace_back(SafeRetain(instance));
}

int32 ParticleEffectComponent::GetEmitterInstanceIndex(ParticleEmitterInstance* emitter) const
{
    for (int32 i = 0, sz = static_cast<int32>(emitterInstances.size()); i < sz; ++i)
    {
        if (emitterInstances[i]->GetEmitter() == emitter->GetEmitter())
        {
            return i;
        }
    }
    return -1;
}

void ParticleEffectComponent::InsertEmitterInstanceAt(ParticleEmitterInstance* emitter, uint32 position)
{
    auto it = emitterInstances.begin();
    std::advance(it, DAVA::Min(position, GetEmittersCount()));
    emitter->SetOwner(this);
    emitterInstances.emplace(it, SafeRetain(emitter));
}

void ParticleEffectComponent::RemoveEmitterInstance(ParticleEmitterInstance* emitter)
{
    auto findPred = [emitter](const DAVA::RefPtr<ParticleEmitterInstance>& qualityEmitter) {
        return (qualityEmitter->GetEmitter() == emitter->GetEmitter());
    };

    auto it = std::find_if(emitterInstances.begin(), emitterInstances.end(), findPred);
    DVASSERT(it != emitterInstances.end());
    emitterInstances.erase(it);
}

/*statistics for editor*/
int32 ParticleEffectComponent::GetLayerActiveParticlesCount(ParticleLayer* layer)
{
    int32 count = 0;
    for (List<ParticleGroup>::iterator it = effectData.groups.begin(), e = effectData.groups.end(); it != e; ++it)
    {
        if (it->layer == layer)
        {
            count += it->activeParticleCount;
        }
    }
    return count;
}
float32 ParticleEffectComponent::GetLayerActiveParticlesSquare(ParticleLayer* layer)
{
    float32 square = 0;
    for (List<ParticleGroup>::iterator it = effectData.groups.begin(), e = effectData.groups.end(); it != e; ++it)
    {
        if (it->layer == layer)
        {
            Particle* currParticle = it->head;
            while (currParticle)
            {
                square += currParticle->currSize.x * currParticle->currSize.y;
                currParticle = currParticle->next;
            }
        }
    }
    return square;
}

float32 ParticleEffectComponent::GetCurrTime()
{
    return time;
}

bool ParticleEffectComponent::GetReflectionVisible() const
{
    return effectRenderObject->GetReflectionVisible();
}
void ParticleEffectComponent::SetReflectionVisible(bool visible)
{
    effectRenderObject->SetReflectionVisible(visible);
}
bool ParticleEffectComponent::GetRefractionVisible() const
{
    return effectRenderObject->GetRefractionVisible();
}
void ParticleEffectComponent::SetRefractionVisible(bool visible)
{
    effectRenderObject->SetRefractionVisible(visible);
}
bool ParticleEffectComponent::GetClippingVisible() const
{
    return effectRenderObject->GetClippingVisible();
}
void ParticleEffectComponent::SetClippingVisible(bool visible)
{
    effectRenderObject->SetClippingVisible(visible);
}

void ParticleEffectComponent::ReloadEmitters()
{
    const ParticlesQualitySettings::FilepathSelector* filepathSelector = QualitySettingsSystem::Instance()->GetParticlesQualitySettings().GetOrCreateFilepathSelector();

    for (auto instance : emitterInstances)
    {
        if (!instance->GetFilePath().IsEmpty())
        {
            FilePath qualityFilepath = instance->GetFilePath();

            if (filepathSelector)
            {
                qualityFilepath = filepathSelector->SelectFilepath(instance->GetFilePath());
            }

            if (qualityFilepath != instance->GetEmitter()->configPath)
            {
                instance->SetEmitter(ParticleEmitter::LoadEmitter(qualityFilepath));
            }
        }
    }
}
}

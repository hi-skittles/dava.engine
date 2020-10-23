#include "Scene3D/Scene.h"

#include "Concurrency/Thread.h"
#include "Debug/ProfilerCPU.h"
#include "Debug/ProfilerMarkerNames.h"
#include "Entity/ComponentUtils.h"
#include "FileSystem/FileSystem.h"
#include "Render/3D/StaticMesh.h"
#include "Render/Highlevel/Landscape.h"
#include "Render/Highlevel/Light.h"
#include "Render/Highlevel/RenderPass.h"
#include "Render/Highlevel/RenderSystem.h"
#include "Render/Highlevel/RenderSystem.h"
#include "Render/Image/Image.h"
#include "Render/MipmapReplacer.h"
#include "Render/RenderOptions.h"
#include "Render/Renderer.h"
#include "Render/Texture.h"
#include "Reflection/ReflectionRegistrator.h"
#include "Scene3D/Components/ComponentHelpers.h"
#include "Scene3D/Components/SingleComponents/MotionSingleComponent.h"
#include "Scene3D/Components/SingleComponents/TransformSingleComponent.h"
#include "Scene3D/Components/TransformComponent.h"
#include "Scene3D/Components/UpdatableComponent.h"
#include "Scene3D/Components/DebugRenderComponent.h"
#include "Scene3D/Components/StaticOcclusionComponent.h"
#include "Scene3D/Components/AnimationComponent.h"
#include "Scene3D/Components/MotionComponent.h"
#include "Scene3D/Components/SlotComponent.h"
#include "Scene3D/Components/SwitchComponent.h"
#include "Scene3D/Components/SoundComponent.h"
#include "Scene3D/Components/RenderComponent.h"
#include "Scene3D/Components/LightComponent.h"
#include "Scene3D/Components/SpeedTreeComponent.h"
#include "Scene3D/Components/WindComponent.h"
#include "Scene3D/Components/WaveComponent.h"
#include "Scene3D/DataNode.h"
#include "Scene3D/Lod/LodComponent.h"
#include "Scene3D/Lod/LodSystem.h"
#include "Scene3D/SceneFileV2.h"
#include "Scene3D/Systems/ActionUpdateSystem.h"
#include "Scene3D/Systems/AnimationSystem.h"
#include "Scene3D/Systems/DebugRenderSystem.h"
#include "Scene3D/Systems/EventSystem.h"
#include "Scene3D/Systems/FoliageSystem.h"
#include "Scene3D/Systems/GeoDecalSystem.h"
#include "Scene3D/Systems/LandscapeSystem.h"
#include "Scene3D/Systems/LightUpdateSystem.h"
#include "Scene3D/Systems/MotionSystem.h"
#include "Scene3D/Systems/ParticleEffectDebugDrawSystem.h"
#include "Scene3D/Systems/ParticleEffectSystem.h"
#include "Scene3D/Systems/RenderUpdateSystem.h"
#include "Scene3D/Systems/SkeletonSystem.h"
#include "Scene3D/Systems/SlotSystem.h"
#include "Scene3D/Systems/SoundUpdateSystem.h"
#include "Scene3D/Systems/SpeedTreeUpdateSystem.h"
#include "Scene3D/Systems/StaticOcclusionSystem.h"
#include "Scene3D/Systems/SwitchSystem.h"
#include "Scene3D/Systems/TransformSystem.h"
#include "Scene3D/Systems/UpdateSystem.h"
#include "Scene3D/Systems/WaveSystem.h"
#include "Scene3D/Systems/WindSystem.h"
#include "Sound/SoundSystem.h"
#include "Time/SystemTimer.h"
#include "UI/UIEvent.h"
#include "Utils/Utils.h"

#if defined(__DAVAENGINE_PHYSICS_DEBUG_DRAW_ENABLED__)
#include "PhysicsDebug/PhysicsDebugDrawSystem.h"
#endif

#if defined(__DAVAENGINE_PHYSICS_ENABLED__)
#include <Physics/WASDPhysicsControllerSystem.h>
#include <Physics/PhysicsSystem.h>
#include <Physics/CollisionSingleComponent.h>
#endif

#include <functional>

namespace DAVA
{
//TODO: remove this crap with shadow color
EntityCache::~EntityCache()
{
    ClearAll();
}

void EntityCache::Preload(const FilePath& path)
{
    Scene* scene = new Scene(0);
    if (SceneFileV2::ERROR_NO_ERROR == scene->LoadScene(path))
    {
        Entity* srcRootEntity = scene;

        // try to perform little optimization:
        // if scene has single node with identity transform
        // we can skip this entity and move only its children
        if (1 == srcRootEntity->GetChildrenCount())
        {
            Entity* child = srcRootEntity->GetChild(0);
            if (1 == child->GetComponentCount())
            {
                TransformComponent* tr = srcRootEntity->GetComponent<TransformComponent>();
                if (nullptr != tr && tr->GetLocalMatrix() == Matrix4::IDENTITY)
                {
                    srcRootEntity = child;
                }
            }
        }

        auto count = srcRootEntity->GetChildrenCount();

        Vector<Entity*> tempV;
        tempV.reserve(count);
        for (auto i = 0; i < count; ++i)
        {
            tempV.push_back(srcRootEntity->GetChild(i));
        }

        Entity* dstRootEntity = new Entity();
        for (auto i = 0; i < count; ++i)
        {
            dstRootEntity->AddNode(tempV[i]);
        }

        dstRootEntity->ResetID();
        dstRootEntity->SetName(scene->GetName());
        cachedEntities[path] = dstRootEntity;
    }

    SafeRelease(scene);
}

Entity* EntityCache::GetOriginal(const FilePath& path)
{
    Entity* ret = nullptr;

    if (cachedEntities.find(path) == cachedEntities.end())
    {
        Preload(path);
    }

    auto i = cachedEntities.find(path);
    if (i != cachedEntities.end())
    {
        ret = i->second;
    }

    return ret;
}

Entity* EntityCache::GetClone(const FilePath& path)
{
    Entity* ret = nullptr;

    Entity* orig = GetOriginal(path);
    if (nullptr != orig)
    {
        ret = orig->Clone();
    }

    return ret;
}

void EntityCache::Clear(const FilePath& path)
{
    auto i = cachedEntities.find(path);
    if (i != cachedEntities.end())
    {
        SafeRelease(i->second);
        cachedEntities.erase(i);
    }
}

void EntityCache::ClearAll()
{
    for (auto& i : cachedEntities)
    {
        SafeRelease(i.second);
    }
    cachedEntities.clear();
}

DAVA_VIRTUAL_REFLECTION_IMPL(Scene)
{
    ReflectionRegistrator<Scene>::Begin()
    .End();
}

Scene::Scene(uint32 _systemsMask /* = SCENE_SYSTEM_ALL_MASK */)
    : Entity()
    , systemsMask(_systemsMask)
    , maxEntityIDCounter(0)
    , sceneGlobalMaterial(0)
    , mainCamera(0)
    , drawCamera(0)
{
    static uint32 idCounter = 0;
    sceneId = ++idCounter;

    CreateComponents();
    CreateSystems();

    // this will force scene to create hidden global material
    SetGlobalMaterial(nullptr);

    RenderOptions* options = Renderer::GetOptions();
    options->AddObserver(this);
}

void Scene::CreateComponents()
{
}

NMaterial* Scene::GetGlobalMaterial() const
{
    return sceneGlobalMaterial;
}

void Scene::SetGlobalMaterial(NMaterial* globalMaterial)
{
    SafeRelease(sceneGlobalMaterial);
    sceneGlobalMaterial = SafeRetain(globalMaterial);

    renderSystem->SetGlobalMaterial(sceneGlobalMaterial);

    if (nullptr != particleEffectSystem)
        particleEffectSystem->SetGlobalMaterial(sceneGlobalMaterial);
}

void Scene::SetMainPassProperties(uint32 priority, const Rect& viewport, uint32 width, uint32 height, PixelFormat format)
{
    renderSystem->SetMainPassProperties(priority, viewport, width, height, format);
}

void Scene::SetMainRenderTarget(rhi::HTexture color, rhi::HTexture depthStencil, rhi::LoadAction colorLoadAction, const Color& clearColor)
{
    renderSystem->SetMainRenderTarget(color, depthStencil, colorLoadAction, clearColor);
}

rhi::RenderPassConfig& Scene::GetMainPassConfig()
{
    return renderSystem->GetMainPassConfig();
}

void Scene::CreateSystems()
{
    renderSystem = new RenderSystem();
    eventSystem = new EventSystem();

    if (SCENE_SYSTEM_STATIC_OCCLUSION_FLAG & systemsMask)
    {
        staticOcclusionSystem = new StaticOcclusionSystem(this);
        AddSystem(staticOcclusionSystem, ComponentUtils::MakeMask<StaticOcclusionDataComponent>(), SCENE_SYSTEM_REQUIRE_PROCESS);
    }

    if (SCENE_SYSTEM_ANIMATION_FLAG & systemsMask)
    {
        animationSystem = new AnimationSystem(this);
        AddSystem(animationSystem, ComponentUtils::MakeMask<AnimationComponent>(), SCENE_SYSTEM_REQUIRE_PROCESS);
    }

    if (SCENE_SYSTEM_MOTION_FLAG & systemsMask)
    {
        motionSingleComponent = new MotionSingleComponent();

        motionSystem = new MotionSystem(this);
        AddSystem(motionSystem, ComponentUtils::MakeMask<SkeletonComponent>() | ComponentUtils::MakeMask<MotionComponent>(), SCENE_SYSTEM_REQUIRE_PROCESS);
    }

#if defined(__DAVAENGINE_PHYSICS_ENABLED__)
    if (SCENE_SYSTEM_PHYSICS_FLAG & systemsMask)
    {
        collisionSingleComponent = new CollisionSingleComponent;

        physicsSystem = new PhysicsSystem(this);
        AddSystem(physicsSystem, 0, SCENE_SYSTEM_REQUIRE_PROCESS);

        WASDPhysicsControllerSystem* wasdPhysicsSystem = new WASDPhysicsControllerSystem(this);
        AddSystem(wasdPhysicsSystem, 0, SCENE_SYSTEM_REQUIRE_PROCESS, physicsSystem, physicsSystem);
    }
#endif

#if defined(__DAVAENGINE_PHYSICS_DEBUG_DRAW_ENABLED__)
    AddSystem(new PhysicsDebugDrawSystem(this), 0, SCENE_SYSTEM_REQUIRE_PROCESS);
#endif

    if (SCENE_SYSTEM_SKELETON_FLAG & systemsMask)
    {
        skeletonSystem = new SkeletonSystem(this);
        AddSystem(skeletonSystem, ComponentUtils::MakeMask<SkeletonComponent>(), SCENE_SYSTEM_REQUIRE_PROCESS);
    }

    if (SCENE_SYSTEM_SLOT_FLAG & systemsMask)
    {
        slotSystem = new SlotSystem(this);
        AddSystem(slotSystem, ComponentUtils::MakeMask<SlotComponent>(), SCENE_SYSTEM_REQUIRE_PROCESS);
    }

    if (SCENE_SYSTEM_TRANSFORM_FLAG & systemsMask)
    {
        transformSystem = new TransformSystem(this);
        AddSystem(transformSystem, ComponentUtils::MakeMask<TransformComponent>(), SCENE_SYSTEM_REQUIRE_PROCESS);

        transformSingleComponent = new TransformSingleComponent;
    }

    if (SCENE_SYSTEM_LOD_FLAG & systemsMask)
    {
        lodSystem = new LodSystem(this);
        AddSystem(lodSystem, ComponentUtils::MakeMask<LodComponent>(), SCENE_SYSTEM_REQUIRE_PROCESS);
    }

    if (SCENE_SYSTEM_SWITCH_FLAG & systemsMask)
    {
        switchSystem = new SwitchSystem(this);
        AddSystem(switchSystem, ComponentUtils::MakeMask<SwitchComponent>(), SCENE_SYSTEM_REQUIRE_PROCESS);
    }

    if (SCENE_SYSTEM_PARTICLE_EFFECT_FLAG & systemsMask)
    {
        particleEffectSystem = new ParticleEffectSystem(this);
        particleEffectSystem->SetGlobalMaterial(GetGlobalMaterial());
        AddSystem(particleEffectSystem, ComponentUtils::MakeMask<ParticleEffectComponent>(), SCENE_SYSTEM_REQUIRE_PROCESS);
    }

    if (SCENE_SYSTEM_SOUND_UPDATE_FLAG & systemsMask)
    {
        soundSystem = new SoundUpdateSystem(this);
        AddSystem(soundSystem, ComponentUtils::MakeMask<TransformComponent>() | ComponentUtils::MakeMask<SoundComponent>(), SCENE_SYSTEM_REQUIRE_PROCESS);
    }

    if (SCENE_SYSTEM_RENDER_UPDATE_FLAG & systemsMask)
    {
        renderUpdateSystem = new RenderUpdateSystem(this);
        AddSystem(renderUpdateSystem, ComponentUtils::MakeMask<TransformComponent>() | ComponentUtils::MakeMask<RenderComponent>(), SCENE_SYSTEM_REQUIRE_PROCESS);
    }

    if (SCENE_SYSTEM_UPDATEBLE_FLAG & systemsMask)
    {
        updatableSystem = new UpdateSystem(this);
        AddSystem(updatableSystem, ComponentUtils::MakeMask<UpdatableComponent>());
    }

    if (SCENE_SYSTEM_LIGHT_UPDATE_FLAG & systemsMask)
    {
        lightUpdateSystem = new LightUpdateSystem(this);
        AddSystem(lightUpdateSystem, ComponentUtils::MakeMask<TransformComponent>() | ComponentUtils::MakeMask<LightComponent>(), SCENE_SYSTEM_REQUIRE_PROCESS);
    }

    if (SCENE_SYSTEM_ACTION_UPDATE_FLAG & systemsMask)
    {
        actionSystem = new ActionUpdateSystem(this);
        AddSystem(actionSystem, ComponentUtils::MakeMask<ActionComponent>(), SCENE_SYSTEM_REQUIRE_PROCESS);
    }

    if (SCENE_SYSTEM_DEBUG_RENDER_FLAG & systemsMask)
    {
        debugRenderSystem = new DebugRenderSystem(this);
        AddSystem(debugRenderSystem, ComponentUtils::MakeMask<DebugRenderComponent>(), SCENE_SYSTEM_REQUIRE_PROCESS);
    }

    if (SCENE_SYSTEM_LANDSCAPE_FLAG & systemsMask)
    {
        landscapeSystem = new LandscapeSystem(this);
        AddSystem(landscapeSystem, ComponentUtils::MakeMask<RenderComponent>(), SCENE_SYSTEM_REQUIRE_PROCESS);
    }

    if (SCENE_SYSTEM_FOLIAGE_FLAG & systemsMask)
    {
        foliageSystem = new FoliageSystem(this);
        AddSystem(foliageSystem, ComponentUtils::MakeMask<RenderComponent>(), SCENE_SYSTEM_REQUIRE_PROCESS);
    }

    if (SCENE_SYSTEM_SPEEDTREE_UPDATE_FLAG & systemsMask)
    {
        speedTreeUpdateSystem = new SpeedTreeUpdateSystem(this);
        AddSystem(speedTreeUpdateSystem, ComponentUtils::MakeMask<SpeedTreeComponent>() | ComponentUtils::MakeMask<RenderComponent>(), SCENE_SYSTEM_REQUIRE_PROCESS);
    }

    if (SCENE_SYSTEM_WIND_UPDATE_FLAG & systemsMask)
    {
        windSystem = new WindSystem(this);
        AddSystem(windSystem, ComponentUtils::MakeMask<WindComponent>(), SCENE_SYSTEM_REQUIRE_PROCESS);
    }

    if (SCENE_SYSTEM_WAVE_UPDATE_FLAG & systemsMask)
    {
        waveSystem = new WaveSystem(this);
        AddSystem(waveSystem, ComponentUtils::MakeMask<WaveComponent>(), SCENE_SYSTEM_REQUIRE_PROCESS);
    }

    if (SCENE_SYSTEM_GEO_DECAL_FLAG & systemsMask)
    {
        geoDecalSystem = new GeoDecalSystem(this);
        AddSystem(geoDecalSystem, ComponentUtils::MakeMask<GeoDecalComponent>(), SCENE_SYSTEM_REQUIRE_PROCESS);
    }

    if (DAVA::Renderer::GetOptions()->IsOptionEnabled(DAVA::RenderOptions::DEBUG_DRAW_STATIC_OCCLUSION) && !staticOcclusionDebugDrawSystem)
    {
        staticOcclusionDebugDrawSystem = new DAVA::StaticOcclusionDebugDrawSystem(this);
        AddSystem(staticOcclusionDebugDrawSystem, ComponentUtils::MakeMask<StaticOcclusionComponent>(), 0, renderUpdateSystem);
    }

    if (DAVA::Renderer::GetOptions()->IsOptionEnabled(RenderOptions::DEBUG_DRAW_PARTICLES) && particleEffectDebugDrawSystem == nullptr)
    {
        particleEffectDebugDrawSystem = new ParticleEffectDebugDrawSystem(this);
        AddSystem(particleEffectDebugDrawSystem, 0);
    }
}

Scene::~Scene()
{
    Renderer::GetOptions()->RemoveObserver(this);

    transformSystem = nullptr;
    renderUpdateSystem = nullptr;
    lodSystem = nullptr;
    debugRenderSystem = nullptr;
    particleEffectSystem = nullptr;
    updatableSystem = nullptr;
    lightUpdateSystem = nullptr;
    switchSystem = nullptr;
    soundSystem = nullptr;
    actionSystem = nullptr;
    staticOcclusionSystem = nullptr;
    speedTreeUpdateSystem = nullptr;
    foliageSystem = nullptr;
    windSystem = nullptr;
    waveSystem = nullptr;
    animationSystem = nullptr;
    motionSystem = nullptr;
#if defined(__DAVAENGINE_PHYSICS_ENABLED__)
    physicsSystem = nullptr;
#endif

    renderSystem->PrepareForShutdown();

    size_t size = systems.size();
    for (size_t k = 0; k < size; ++k)
    {
        systems[k]->PrepareForRemove();
    }

    for (size_t k = 0; k < size; ++k)
    {
        SafeDelete(systems[k]);
    }
    systems.clear();

    SafeRelease(mainCamera);
    SafeRelease(drawCamera);
    for (Camera*& c : cameras)
        SafeRelease(c);
    cameras.clear();

    for (SingletonComponent* s : singletonComponents)
    {
        SafeDelete(s);
    }
    singletonComponents.clear();

    SafeDelete(transformSingleComponent);
    SafeDelete(motionSingleComponent);
#if defined(__DAVAENGINE_PHYSICS_ENABLED__)
    SafeDelete(collisionSingleComponent);
#endif

    systemsToProcess.clear();
    systemsToInput.clear();
    systemsToFixedProcess.clear();

    RemoveAllChildren();
    SafeRelease(sceneGlobalMaterial);

    cache.ClearAll();

    SafeDelete(eventSystem);
    SafeDelete(renderSystem);
}

void Scene::RegisterEntity(Entity* entity)
{
    if (entity->GetID() == 0 ||
        entity->GetSceneID() == 0 ||
        entity->GetSceneID() != sceneId)
    {
        entity->SetID(++maxEntityIDCounter);
        entity->SetSceneID(sceneId);
    }

    for (auto& system : systems)
    {
        system->RegisterEntity(entity);
    }
}

void Scene::UnregisterEntity(Entity* entity)
{
    if (transformSingleComponent)
    {
        transformSingleComponent->EraseEntity(entity);
    }
    if (motionSingleComponent)
    {
        motionSingleComponent->EntityRemoved(entity);
    }

#if defined(__DAVAENGINE_PHYSICS_ENABLED__)
    if (collisionSingleComponent)
    {
        collisionSingleComponent->RemoveCollisionsWithEntity(entity);
    }
#endif

    for (auto& system : systems)
    {
        system->UnregisterEntity(entity);
    }
}

void Scene::RegisterEntitiesInSystemRecursively(SceneSystem* system, Entity* entity)
{
    system->RegisterEntity(entity);
    for (int32 i = 0, sz = entity->GetChildrenCount(); i < sz; ++i)
        RegisterEntitiesInSystemRecursively(system, entity->GetChild(i));
}

void Scene::RegisterComponent(Entity* entity, Component* component)
{
    DVASSERT(entity && component);
    uint32 systemsCount = static_cast<uint32>(systems.size());
    for (uint32 k = 0; k < systemsCount; ++k)
    {
        systems[k]->RegisterComponent(entity, component);
    }
}

void Scene::UnregisterComponent(Entity* entity, Component* component)
{
    DVASSERT(entity && component);
    uint32 systemsCount = static_cast<uint32>(systems.size());
    for (uint32 k = 0; k < systemsCount; ++k)
    {
        systems[k]->UnregisterComponent(entity, component);
    }
}

void Scene::AddSystem(SceneSystem* sceneSystem, const ComponentMask& componentMask, uint32 processFlags /*= 0*/, SceneSystem* insertBeforeSceneForProcess /* = nullptr */, SceneSystem* insertBeforeSceneForInput /* = nullptr*/, SceneSystem* insertBeforeSceneForFixedProcess)
{
    sceneSystem->SetRequiredComponents(componentMask);
    systems.push_back(sceneSystem);

    auto insertSystemBefore = [sceneSystem](Vector<SceneSystem*>& container, SceneSystem* beforeThisSystem)
    {
        if (beforeThisSystem != nullptr)
        {
            Vector<SceneSystem*>::iterator itEnd = container.end();
            for (Vector<SceneSystem*>::iterator it = container.begin(); it != itEnd; ++it)
            {
                if (beforeThisSystem == (*it))
                {
                    container.insert(it, sceneSystem);
                    return true;
                }
            }
        }
        else
        {
            container.push_back(sceneSystem);
            return true;
        }

        return false;
    };

    if (processFlags & SCENE_SYSTEM_REQUIRE_PROCESS)
    {
        bool wasInsertedForProcess = insertSystemBefore(systemsToProcess, insertBeforeSceneForProcess);
        DVASSERT(wasInsertedForProcess);
    }

    if (processFlags & SCENE_SYSTEM_REQUIRE_INPUT)
    {
        bool wasInsertedForInput = insertSystemBefore(systemsToInput, insertBeforeSceneForInput);
        DVASSERT(wasInsertedForInput);
    }

    if (processFlags & SCENE_SYSTEM_REQUIRE_FIXED_PROCESS)
    {
        bool wasInserted = insertSystemBefore(systemsToProcess, insertBeforeSceneForFixedProcess);
        DVASSERT(wasInserted);
    }

    sceneSystem->SetScene(this);
    RegisterEntitiesInSystemRecursively(sceneSystem, this);
}

void Scene::RemoveSystem(SceneSystem* sceneSystem)
{
    sceneSystem->PrepareForRemove();

    RemoveSystem(systemsToProcess, sceneSystem);
    RemoveSystem(systemsToInput, sceneSystem);
    RemoveSystem(systemsToFixedProcess, sceneSystem);

    bool removed = RemoveSystem(systems, sceneSystem);
    if (removed)
    {
        sceneSystem->SetScene(nullptr);
    }
    else
    {
        DVASSERT(false, "Failed to remove system from scene");
    }
}

bool Scene::RemoveSystem(Vector<SceneSystem*>& storage, SceneSystem* system)
{
    Vector<SceneSystem*>::iterator endIt = storage.end();
    for (Vector<SceneSystem*>::iterator it = storage.begin(); it != endIt; ++it)
    {
        if (*it == system)
        {
            storage.erase(it);
            return true;
        }
    }

    return false;
}

void Scene::AddSingletonComponent(SingletonComponent* component)
{
    DVASSERT(GetSingletonComponent<std::remove_pointer<decltype(component)>::type>() == nullptr);
    singletonComponents.push_back(component);
}

void Scene::RemoveSingletonComponent(SingletonComponent* component)
{
    FindAndRemoveExchangingWithLast(singletonComponents, component);
}

Scene* Scene::GetScene()
{
    return this;
}

void Scene::AddCamera(Camera* camera)
{
    if (camera)
    {
        camera->Retain();
        cameras.push_back(camera);
    }
}

bool Scene::RemoveCamera(Camera* c)
{
    const auto& it = std::find(cameras.begin(), cameras.end(), c);
    if (it != cameras.end())
    {
        SafeRelease(*it);
        cameras.erase(it);
        return true;
    }
    return false;
}

Camera* Scene::GetCamera(int32 n)
{
    if (n >= 0 && n < static_cast<int32>(cameras.size()))
        return cameras[n];

    return nullptr;
}

void Scene::Update(float32 timeElapsed)
{
    DAVA_PROFILER_CPU_SCOPE(ProfilerCPUMarkerName::SCENE_UPDATE)

    fixedUpdate.lastTime += timeElapsed;
    //call ProcessFixed N times where N = (timeSinceLastProcessFixed + timeElapsed) / fixedUpdate.constantTime;
    while (fixedUpdate.lastTime >= fixedUpdate.constantTime)
    {
        for (SceneSystem* system : systemsToFixedProcess)
        {
            system->ProcessFixed(fixedUpdate.constantTime);
        }
        fixedUpdate.lastTime -= fixedUpdate.constantTime;
    }

    for (SceneSystem* system : systemsToProcess)
    {
        if ((systemsMask & SCENE_SYSTEM_UPDATEBLE_FLAG) && system == transformSystem)
        {
            updatableSystem->UpdatePreTransform(timeElapsed);
            transformSystem->Process(timeElapsed);
            updatableSystem->UpdatePostTransform(timeElapsed);
        }
        else if (system == lodSystem)
        {
            if (Renderer::GetOptions()->IsOptionEnabled(RenderOptions::UPDATE_LODS))
            {
                lodSystem->Process(timeElapsed);
            }
        }
        else
        {
            system->Process(timeElapsed);
        }
    }

    if (transformSingleComponent)
    {
        transformSingleComponent->Clear();
    }

#if defined(__DAVAENGINE_PHYSICS_ENABLED__)
    if (collisionSingleComponent)
    {
        collisionSingleComponent->collisions.clear();
    }
#endif

    sceneGlobalTime += timeElapsed;
}

void Scene::Draw()
{
    DAVA_PROFILER_CPU_SCOPE(ProfilerCPUMarkerName::SCENE_DRAW)

    //TODO: re-think configuring global dynamic bindings
    static Color defShadowColor(1.f, 0.f, 0.f, 1.f);
    static Color defWaterClearColor(0.f, 0.f, 0.f, 0.f);

    const float32* shadowDataPtr = defShadowColor.color;
    const float32* waterDataPtr = defWaterClearColor.color;
    if (sceneGlobalMaterial && sceneGlobalMaterial->HasLocalProperty(DAVA::NMaterialParamName::DEPRECATED_SHADOW_COLOR_PARAM))
        shadowDataPtr = sceneGlobalMaterial->GetLocalPropValue(DAVA::NMaterialParamName::DEPRECATED_SHADOW_COLOR_PARAM);
    if (sceneGlobalMaterial && sceneGlobalMaterial->HasLocalProperty(DAVA::NMaterialParamName::WATER_CLEAR_COLOR))
        waterDataPtr = sceneGlobalMaterial->GetLocalPropValue(DAVA::NMaterialParamName::WATER_CLEAR_COLOR);

    Renderer::GetDynamicBindings().SetDynamicParam(DynamicBindings::PARAM_SHADOW_COLOR, shadowDataPtr, reinterpret_cast<pointer_size>(shadowDataPtr));
    Renderer::GetDynamicBindings().SetDynamicParam(DynamicBindings::PARAM_WATER_CLEAR_COLOR, waterDataPtr, reinterpret_cast<pointer_size>(waterDataPtr));
    Renderer::GetDynamicBindings().SetDynamicParam(DynamicBindings::PARAM_GLOBAL_TIME, &sceneGlobalTime, reinterpret_cast<pointer_size>(&sceneGlobalTime));

    renderSystem->Render();

    if (particleEffectDebugDrawSystem != nullptr)
        particleEffectDebugDrawSystem->Draw();
}

void Scene::SceneDidLoaded()
{
    maxEntityIDCounter = 0;

    std::function<void(Entity*)> findMaxId = [&](Entity* entity)
    {
        if (maxEntityIDCounter < entity->id)
            maxEntityIDCounter = entity->id;
        for (auto child : entity->children) findMaxId(child);
    };

    findMaxId(this);

    uint32 systemsCount = static_cast<uint32>(systems.size());
    for (uint32 k = 0; k < systemsCount; ++k)
    {
        systems[k]->SceneDidLoaded();
    }
}

void Scene::SetCurrentCamera(Camera* _camera)
{
    SafeRelease(mainCamera);
    mainCamera = SafeRetain(_camera);
    SafeRelease(drawCamera);
    drawCamera = SafeRetain(_camera);
}

Camera* Scene::GetCurrentCamera() const
{
    return mainCamera;
}

void Scene::SetCustomDrawCamera(Camera* _camera)
{
    SafeRelease(drawCamera);
    drawCamera = SafeRetain(_camera);
}

Camera* Scene::GetDrawCamera() const
{
    return drawCamera;
}

EventSystem* Scene::GetEventSystem() const
{
    return eventSystem;
}

RenderSystem* Scene::GetRenderSystem() const
{
    return renderSystem;
}

AnimationSystem* Scene::GetAnimationSystem() const
{
    return animationSystem;
}

ParticleEffectDebugDrawSystem* Scene::GetParticleEffectDebugDrawSystem() const
{
    return particleEffectDebugDrawSystem;
}

SceneFileV2::eError Scene::LoadScene(const DAVA::FilePath& pathname)
{
    SceneFileV2::eError ret = SceneFileV2::ERROR_FAILED_TO_CREATE_FILE;

    RemoveAllChildren();
    SetName(pathname.GetFilename().c_str());

    if (pathname.IsEqualToExtension(".sc2"))
    {
        ScopedPtr<SceneFileV2> file(new SceneFileV2());
        file->EnableDebugLog(false);
        ret = file->LoadScene(pathname, this);
    }

    return ret;
}

SceneFileV2::eError Scene::SaveScene(const DAVA::FilePath& pathname, bool saveForGame /*= false*/)
{
    std::function<void(Entity*)> resolveId = [&](Entity* entity)
    {
        if (0 == entity->id)
            entity->id = ++maxEntityIDCounter;
        for (auto child : entity->children) resolveId(child);
    };

    resolveId(this);

    ScopedPtr<SceneFileV2> file(new SceneFileV2());
    file->EnableDebugLog(false);
    file->EnableSaveForGame(saveForGame);
    return file->SaveScene(pathname, this);
}

void Scene::OptimizeBeforeExport()
{
    List<NMaterial*> materials;
    GetDataNodes(materials);

    const auto RemoveMaterialFlag = [](NMaterial* material, const FastName& flagName) {
        if (material->HasLocalFlag(flagName))
        {
            material->RemoveFlag(flagName);
        }
    };

    for (auto& mat : materials)
    {
        RemoveMaterialFlag(mat, NMaterialFlagName::FLAG_ILLUMINATION_USED);
        RemoveMaterialFlag(mat, NMaterialFlagName::FLAG_ILLUMINATION_SHADOW_CASTER);
        RemoveMaterialFlag(mat, NMaterialFlagName::FLAG_ILLUMINATION_SHADOW_RECEIVER);

        if (mat->HasLocalProperty(NMaterialParamName::PARAM_LIGHTMAP_SIZE))
        {
            mat->RemoveProperty(NMaterialParamName::PARAM_LIGHTMAP_SIZE);
        }
    }

    Entity::OptimizeBeforeExport();
}

void Scene::OnSceneReady(Entity* rootNode)
{
}

void Scene::Input(DAVA::UIEvent* event)
{
    for (SceneSystem* system : systemsToInput)
    {
        system->Input(event);
    }
}

void Scene::InputCancelled(UIEvent* event)
{
    for (SceneSystem* system : systemsToInput)
    {
        system->InputCancelled(event);
    }
}

void Scene::HandleEvent(Observable* observable)
{
    RenderOptions* options = dynamic_cast<RenderOptions*>(observable);

    if (options->IsOptionEnabled(RenderOptions::REPLACE_LIGHTMAP_MIPMAPS))
        MipMapReplacer::ReplaceMipMaps(this, NMaterialTextureName::TEXTURE_LIGHTMAP);
    if (options->IsOptionEnabled(RenderOptions::REPLACE_ALBEDO_MIPMAPS))
        MipMapReplacer::ReplaceMipMaps(this, NMaterialTextureName::TEXTURE_ALBEDO);

    if (options->IsOptionEnabled(RenderOptions::DEBUG_DRAW_STATIC_OCCLUSION) && !staticOcclusionDebugDrawSystem)
    {
        staticOcclusionDebugDrawSystem = new StaticOcclusionDebugDrawSystem(this);
        AddSystem(staticOcclusionDebugDrawSystem, ComponentUtils::MakeMask<StaticOcclusionComponent>(), 0, renderUpdateSystem);
    }
    else if (!options->IsOptionEnabled(RenderOptions::DEBUG_DRAW_STATIC_OCCLUSION) && staticOcclusionDebugDrawSystem)
    {
        RemoveSystem(staticOcclusionDebugDrawSystem);
        SafeDelete(staticOcclusionDebugDrawSystem);
    }

    if (DAVA::Renderer::GetOptions()->IsOptionEnabled(RenderOptions::DEBUG_DRAW_PARTICLES) && particleEffectDebugDrawSystem == nullptr)
    {
        particleEffectDebugDrawSystem = new ParticleEffectDebugDrawSystem(this);
        AddSystem(particleEffectDebugDrawSystem, 0);
    }
    else if (!DAVA::Renderer::GetOptions()->IsOptionEnabled(RenderOptions::DEBUG_DRAW_PARTICLES) && particleEffectDebugDrawSystem != nullptr)
    {
        RemoveSystem(particleEffectDebugDrawSystem);
        SafeDelete(particleEffectDebugDrawSystem);
    }
}

void Scene::Activate()
{
    for (auto system : systems)
    {
        system->Activate();
    }
}

void Scene::Deactivate()
{
    for (auto system : systems)
    {
        system->Deactivate();
    }
}
};

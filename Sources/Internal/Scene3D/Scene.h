#pragma once

#include "Base/BaseMath.h"
#include "Base/BaseTypes.h"
#include "Base/Observer.h"
#include "Entity/SceneSystem.h"
#include "Entity/SingletonComponent.h"
#include "Render/Highlevel/Camera.h"
#include "Render/Highlevel/Light.h"
#include "Reflection/Reflection.h"
#include "Render/RenderBase.h"
#include "Scene3D/Entity.h"
#include "Scene3D/SceneFile/SerializationContext.h"
#include "Scene3D/SceneFile/VersionInfo.h"
#include "Scene3D/SceneFileV2.h"

namespace DAVA
{
/**
    \defgroup scene3d 3D Engine
  */

class Texture;
class StaticMesh;
class DataNode;
class ShadowVolumeNode;
class Light;
class ShadowRect;
class QuadTree;
class Component;
class RenderSystem;
class RenderUpdateSystem;
class TransformSystem;
class DebugRenderSystem;
class EventSystem;
class ParticleEffectSystem;
class UpdateSystem;
class LightUpdateSystem;
class SwitchSystem;
class SoundUpdateSystem;
class ActionUpdateSystem;
class StaticOcclusionSystem;
class StaticOcclusionDebugDrawSystem;
class SpeedTreeUpdateSystem;
class FoliageSystem;
class WindSystem;
class WaveSystem;
class SkeletonSystem;
class MotionSystem;
class AnimationSystem;
class LandscapeSystem;
class LodSystem;
class ParticleEffectDebugDrawSystem;
class GeoDecalSystem;
class SlotSystem;
class TransformSingleComponent;
class MotionSingleComponent;
class PhysicsSystem;
class CollisionSingleComponent;

class UIEvent;
class RenderPass;

/**
    \ingroup scene3d
    \brief This class is a code of our 3D Engine scene graph. 
    To visualize any 3d scene you'll need to create Scene object. 
    Scene have visible hierarchy and invisible root nodes. You can add as many root nodes as you want, and do not visualize them.
    For example you can have multiple scenes, load them to one scene, and show each scene when it will be required. 
 */
class EntityCache
{
public:
    ~EntityCache();

    void Preload(const FilePath& path);
    void Clear(const FilePath& path);
    void ClearAll();

    Entity* GetOriginal(const FilePath& path);
    Entity* GetClone(const FilePath& path);

protected:
    Map<FilePath, Entity*> cachedEntities;
};

class Scene : public Entity, Observer
{
protected:
    virtual ~Scene();

public:
    enum : uint32
    {
        SCENE_SYSTEM_TRANSFORM_FLAG = 1 << 0,
        SCENE_SYSTEM_RENDER_UPDATE_FLAG = 1 << 1,
        SCENE_SYSTEM_LOD_FLAG = 1 << 2,
        SCENE_SYSTEM_DEBUG_RENDER_FLAG = 1 << 3,
        SCENE_SYSTEM_PARTICLE_EFFECT_FLAG = 1 << 4,
        SCENE_SYSTEM_UPDATEBLE_FLAG = 1 << 5,
        SCENE_SYSTEM_LIGHT_UPDATE_FLAG = 1 << 6,
        SCENE_SYSTEM_SWITCH_FLAG = 1 << 7,
        SCENE_SYSTEM_SOUND_UPDATE_FLAG = 1 << 8,
        SCENE_SYSTEM_ACTION_UPDATE_FLAG = 1 << 9,
        SCENE_SYSTEM_STATIC_OCCLUSION_FLAG = 1 << 11,
        SCENE_SYSTEM_LANDSCAPE_FLAG = 1 << 12,
        SCENE_SYSTEM_FOLIAGE_FLAG = 1 << 13,
        SCENE_SYSTEM_SPEEDTREE_UPDATE_FLAG = 1 << 14,
        SCENE_SYSTEM_WIND_UPDATE_FLAG = 1 << 15,
        SCENE_SYSTEM_WAVE_UPDATE_FLAG = 1 << 16,
        SCENE_SYSTEM_SKELETON_FLAG = 1 << 17,
        SCENE_SYSTEM_ANIMATION_FLAG = 1 << 18,
        SCENE_SYSTEM_SLOT_FLAG = 1 << 19,
        SCENE_SYSTEM_MOTION_FLAG = 1 << 20,
        SCENE_SYSTEM_GEO_DECAL_FLAG = 1 << 21,

#if defined(__DAVAENGINE_PHYSICS_ENABLED__)
        SCENE_SYSTEM_PHYSICS_FLAG = 1 << 19,
#endif
        SCENE_SYSTEM_ALL_MASK = 0xFFFFFFFF
    };

    enum eSceneProcessFlags : uint32
    {
        SCENE_SYSTEM_REQUIRE_PROCESS = 1 << 0,
        SCENE_SYSTEM_REQUIRE_INPUT = 1 << 1,
        SCENE_SYSTEM_REQUIRE_FIXED_PROCESS = 1 << 2
    };

    Scene(uint32 systemsMask = SCENE_SYSTEM_ALL_MASK);

    /**
        \brief Function to register entity in scene. This function is called when you add entity to scene.
     */
    void RegisterEntity(Entity* entity);
    /**
        \brief Function to unregister entity from scene. This function is called when you remove entity from scene.
     */
    void UnregisterEntity(Entity* entity);

    /**
        \brief Function to register component in scene. This function is called when you add any component to any entity in scene.
     */
    void RegisterComponent(Entity* entity, Component* component);
    /**
        \brief Function to unregister component from scene. This function is called when you remove any component from any entity in scene.
     */
    void UnregisterComponent(Entity* entity, Component* component);

    virtual void AddSystem(SceneSystem* sceneSystem, const ComponentMask& componentMask, uint32 processFlags = 0, SceneSystem* insertBeforeSceneForProcess = nullptr, SceneSystem* insertBeforeSceneForInput = nullptr, SceneSystem* insertBeforeSceneForFixedProcess = nullptr);
    virtual void RemoveSystem(SceneSystem* sceneSystem);
    template <class T>
    T* GetSystem();

    Vector<SceneSystem*> systems;
    Vector<SceneSystem*> systemsToProcess;
    Vector<SceneSystem*> systemsToInput;
    Vector<SceneSystem*> systemsToFixedProcess;

    TransformSystem* transformSystem = nullptr;
    RenderUpdateSystem* renderUpdateSystem = nullptr;
    LodSystem* lodSystem = nullptr;
    DebugRenderSystem* debugRenderSystem = nullptr;
    EventSystem* eventSystem = nullptr;
    ParticleEffectSystem* particleEffectSystem = nullptr;
    UpdateSystem* updatableSystem = nullptr;
    LightUpdateSystem* lightUpdateSystem = nullptr;
    SwitchSystem* switchSystem = nullptr;
    RenderSystem* renderSystem = nullptr;
    SoundUpdateSystem* soundSystem = nullptr;
    ActionUpdateSystem* actionSystem = nullptr;
    StaticOcclusionSystem* staticOcclusionSystem = nullptr;
    SpeedTreeUpdateSystem* speedTreeUpdateSystem = nullptr;
    FoliageSystem* foliageSystem = nullptr;
    VersionInfo::SceneVersion version;
    WindSystem* windSystem = nullptr;
    WaveSystem* waveSystem = nullptr;
    AnimationSystem* animationSystem = nullptr;
    StaticOcclusionDebugDrawSystem* staticOcclusionDebugDrawSystem = nullptr;
    SkeletonSystem* skeletonSystem = nullptr;
    MotionSystem* motionSystem = nullptr;
    LandscapeSystem* landscapeSystem = nullptr;
    ParticleEffectDebugDrawSystem* particleEffectDebugDrawSystem = nullptr;
    SlotSystem* slotSystem = nullptr;
    GeoDecalSystem* geoDecalSystem = nullptr;
    PhysicsSystem* physicsSystem = nullptr;

    CollisionSingleComponent* collisionSingleComponent = nullptr;
    TransformSingleComponent* transformSingleComponent = nullptr;
    MotionSingleComponent* motionSingleComponent = nullptr;

    void AddSingletonComponent(SingletonComponent* component);
    template <class T>
    T* GetSingletonComponent();
    void RemoveSingletonComponent(SingletonComponent* component);
    Vector<SingletonComponent*> singletonComponents;

    /**
        \brief Overloaded GetScene returns this, instead of normal functionality.
     */
    Scene* GetScene() override;

    void HandleEvent(Observable* observable) override; //Handle RenderOptions

    //virtual void StopAllAnimations(bool recursive = true);

    virtual void Update(float32 timeElapsed);
    virtual void Draw();
    void SceneDidLoaded() override;

    Camera* GetCamera(int32 n);
    void AddCamera(Camera* c);
    bool RemoveCamera(Camera* c);
    inline int32 GetCameraCount();

    void SetCurrentCamera(Camera* camera);
    Camera* GetCurrentCamera() const;

    /* 
        This camera is used for visualization setup only. Most system functions use mainCamere, draw camera is used to setup matrices for render. If you do not call this function GetDrawCamera returns currentCamera. 
        You can use SetCustomDrawCamera function if you want to test frustum clipping, and view the scene from different angles.
     */
    void SetCustomDrawCamera(Camera* camera);
    Camera* GetDrawCamera() const;

    void CreateComponents();
    void CreateSystems();

    EventSystem* GetEventSystem() const;
    RenderSystem* GetRenderSystem() const;
    AnimationSystem* GetAnimationSystem() const;
    ParticleEffectDebugDrawSystem* GetParticleEffectDebugDrawSystem() const;

    virtual SceneFileV2::eError LoadScene(const DAVA::FilePath& pathname);
    virtual SceneFileV2::eError SaveScene(const DAVA::FilePath& pathname, bool saveForGame = false);

    virtual void OptimizeBeforeExport();

    DAVA::NMaterial* GetGlobalMaterial() const;
    void SetGlobalMaterial(DAVA::NMaterial* globalMaterial);

    void OnSceneReady(Entity* rootNode);

    void Input(UIEvent* event);
    void InputCancelled(UIEvent* event);

    /**
        \brief This functions activate and deactivate scene systems
     */
    virtual void Activate();
    virtual void Deactivate();

    EntityCache cache;

    void SetMainPassProperties(uint32 priority, const Rect& viewport, uint32 width, uint32 height, PixelFormat format);
    void SetMainRenderTarget(rhi::HTexture color, rhi::HTexture depthStencil, rhi::LoadAction colorLoadAction, const Color& clearColor);

public: // deprecated methods
    DAVA_DEPRECATED(rhi::RenderPassConfig& GetMainPassConfig());

protected:
    void RegisterEntitiesInSystemRecursively(SceneSystem* system, Entity* entity);

    bool RemoveSystem(Vector<SceneSystem*>& storage, SceneSystem* system);

    uint32 systemsMask;
    uint32 maxEntityIDCounter;

    float32 sceneGlobalTime = 0.f;

    Vector<Camera*> cameras;

    NMaterial* sceneGlobalMaterial;

    Camera* mainCamera;
    Camera* drawCamera;

    struct FixedUpdate
    {
        float32 constantTime = 0.016f;
        float32 lastTime = 0.f;
    } fixedUpdate;

    friend class Entity;
    DAVA_VIRTUAL_REFLECTION(Scene, Entity);
};

template <class T>
T* Scene::GetSystem()
{
    T* res = nullptr;
    const std::type_info& type = typeid(T);
    for (SceneSystem* system : systems)
    {
        const std::type_info& currType = typeid(*system);
        if (currType == type)
        {
            res = static_cast<T*>(system);
            break;
        }
    }

    return res;
}

template <class T>
T* Scene::GetSingletonComponent()
{
    T* res = nullptr;
    const std::type_info& type = typeid(T);
    for (SingletonComponent* component : singletonComponents)
    {
        const std::type_info& currType = typeid(*component);
        if (currType == type)
        {
            res = static_cast<T*>(component);
            break;
        }
    }

    return res;
}

int32 Scene::GetCameraCount()
{
    return static_cast<int32>(cameras.size());
}
};

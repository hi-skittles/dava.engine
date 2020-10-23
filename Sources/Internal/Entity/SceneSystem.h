#pragma once

#include "Base/BaseTypes.h"

/**
    \defgroup systems Systems
*/

namespace DAVA
{
class Entity;
class Scene;
class Component;
class UIEvent;
/**
    \ingroup systems
    \brief Base class of systems.
*/
class SceneSystem
{
public:
    SceneSystem(Scene* scene);
    virtual ~SceneSystem() = default;

    inline void SetRequiredComponents(const ComponentMask& requiredComponents);
    inline const ComponentMask& GetRequiredComponents() const;

    /**
        \brief  This function is called when any entity registered to scene.
                It sorts out is entity has all necessary components and we need to call AddEntity.
        \param[in] entity entity we've just added
     */
    virtual void RegisterEntity(Entity* entity);
    /**
        \brief  This function is called when any entity unregistered from scene.
                It sorts out is entity has all necessary components and we need to call RemoveEntity.
        \param[in] entity entity we've just removed
     */
    virtual void UnregisterEntity(Entity* entity);

    /**
        \brief  This function is called when any component is registered to scene.
                It sorts out is entity has all necessary components and we need to call AddEntity.
        \param[in] entity entity we added component to.
        \param[in] component component we've just added to entity.
     */
    virtual void RegisterComponent(Entity* entity, Component* component);

    /**
        \brief  This function is called when any component is unregistered from scene.
                It sorts out is entity has all necessary components and we need to call RemoveEntity.
        \param[in] entity entity we removed component from.
        \param[in] component component we've just removed from entity.
     */
    virtual void UnregisterComponent(Entity* entity, Component* component);

    /**
        \brief  This function can check is (entity, component) pair fits current system required components, if we want to add or remove this component now.
        \param[in] entity entity to check.
        \param[in] component component we want to add or remove.
     */
    bool IsEntityComponentFitsToSystem(Entity* entity, Component* component);

    /**
        \brief This function is called only when entity has all required components.
        \param[in] entity entity we want to add.
     */
    virtual void AddEntity(Entity* entity);

    /**
        \brief This function is called only when entity had all required components, and don't have them anymore.
        \param[in] entity entity we want to remove.
     */
    virtual void RemoveEntity(Entity* entity);

    /*
        Left these callbacks to full compatibility with old multicomponent solution. Probably will be removed later, if we decide to get rid of multicomponents.
     */
    virtual void AddComponent(Entity* entity, Component* component);
    virtual void RemoveComponent(Entity* entity, Component* component);

    /**
        \brief This function is called when scene loading did finished.
     */
    virtual void SceneDidLoaded();

    /**
        /brief This function is called before system will be removed from scene
               instead of unregister all scene's entities.
    */
    virtual void PrepareForRemove() = 0;

    /**
        \brief This function is called when event is fired to this system.
        \param[in] entity entity fired an event.
        \param[in] event event id for this event.
     */
    virtual void ImmediateEvent(Component* component, uint32 event);
    /**
        \brief This function should be overloaded and perform all processing for this system.
        \param[in] timeElapsed time elapsed from previous frame.
     */
    virtual void Process(float32 timeElapsed);

    /** Similar to 'Process' but called with fixed 'timeElapsed'. Could be called from zero to several times per frame. */
    virtual void ProcessFixed(float32 timeElapsed){};

    virtual bool Input(UIEvent* uie)
    {
        return false;
    }

    virtual void InputCancelled(UIEvent* event);

    virtual void SetLocked(bool locked);
    bool IsLocked() const;

    /**
         \brief This functions should be overloaded if system need to do specific actions on scene activation or deactivation 
     */
    virtual void Activate()
    {
    }
    virtual void Deactivate()
    {
    }

protected:
    friend class Scene;

    Scene* GetScene() const;
    virtual void SetScene(Scene* scene);

private:
    ComponentMask requiredComponents;
    Scene* scene = nullptr;

    bool locked = false;
};

// Inline
inline Scene* SceneSystem::GetScene() const
{
    return scene;
}

inline void SceneSystem::SetRequiredComponents(const ComponentMask& requiredComponents_)
{
    requiredComponents = requiredComponents_;
}

inline const ComponentMask& SceneSystem::GetRequiredComponents() const
{
    return requiredComponents;
}
}

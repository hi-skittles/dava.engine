#pragma once

#include "Base/Type.h"
#include "Base/BaseTypes.h"
#include "Entity/ComponentManager.h"

namespace DAVA
{
class Component;

/** Component class utils. (Only for scene components for the moment.) */
class ComponentUtils
{
public:
    /**
        Return ptr to new component of `type`.
        `nullptr` is returned in case if `type` is not registered in ComponentManager.
    */
    static Component* Create(const Type* type);

    /**
        Return ptr to new component of type corresponding to `runtimeId`.
        `nullptr` is returned in case if type for `runtimeId` is not registered in ComponentManager.
    */
    static Component* Create(uint32 runtimeId);

    //------------------------------------------------------------------------------------------------------------------------

    /**
        Return runtime id for `T`.
        Behavior is undefined if `T` is not registered in ComponentManager.
    */
    template <typename T>
    static uint32 GetRuntimeId();

    /**
        Return runtime id for `type`.
        Behavior is undefined if `type` is not registered in ComponentManager.
    */
    static uint32 GetRuntimeId(const Type* type);

    //------------------------------------------------------------------------------------------------------------------------

    /**
        Return ComponentMask with flags corresponding to each type in Args set to true.
        Behavior is undefined if some types are not registered in ComponentManager.
    */
    template <typename... Args>
    static ComponentMask MakeMask();

    /**
        Return ComponentMask with flags corresponding to each Type in Args set to true.
        Behavior is undefined if some Types are not registered in ComponentManager.
    */
    template <typename... Args>
    static ComponentMask MakeMask(const Args*... args);

    //------------------------------------------------------------------------------------------------------------------------

    /**
        Return type for given `runtimeId`.
        Return nullptr if component with 'runtimeId' was not registered in ComponentManager.
        (Just a shortcut for ComponentManager::GetSceneComponentType.)
    */
    static const Type* GetType(uint32 runtimeId);

    /**
        Return sorted runtime component id, based on permanent name.
        Behavior is undefined if `type` is not registered in ComponentManager.
        (Use cases are very rare, think twice before usage.)
    */
    static uint32 GetSortedId(const Type* type);

private:
    friend class ComponentManager;

    static ComponentManager* componentManager;
};

} // namespace DAVA

#include "Entity/Private/ComponentUtils_impl.h"
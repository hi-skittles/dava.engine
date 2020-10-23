#pragma once

#include "Base/Type.h"
#include "Base/BaseTypes.h"

namespace DAVA
{
/**
    Tracks Components/UIComponents types.
    Before any Component/UIComponent is used, it must be added to both Reflection(through registering Reflection permanent name) and ComponentManager(through ComponentManager::RegisterComponent).

    For example:
    \code
        DAVA_REFLECTION_REGISTER_CUSTOM_PERMANENT_NAME(UIFlowLayoutComponent, "FlowLayout");
        GetEngineContext()->componentManager->RegisterComponent<UIFlowLayoutComponent>();
    \endcode

    Base engine Components are registered automatically (if both reflection and permanent name are provided).
    Base engine UIComponents are registered in ReflectionDeclaration.cpp (for now).

    After registration, Component/UIComponent can be created through ComponentUtils::Create/UIComponent::CreateByType(const Type* componentType).

    Component/UIComponent registration also introduces 'runtimeComponentId' (just integer).
    'runtimeComponentId' can be used for Component/UIComponents management along with its' Type. 'runtimeComponentId' is typically used for optimization (for example, indices in array).
    You cannot rely on its actual value between launches of the application.
*/

class UIComponent;
class Component;

namespace Private
{
class EngineBackend;
}

class ComponentManager
{
    friend class Private::EngineBackend; // For creation

public:
    /** Register new component of specified type 'T'. The behavior is undefined until 'T' is derived from Component/UIComponent. */
    template <typename T>
    void RegisterComponent();

    /** Register new component of specified `Type`. The behavior is undefined until `type` is derived from Component/UIComponent. */
    void RegisterComponent(const Type* type);

    /**
        Return CRC32 hash of preregistered scene components.
        (Hash of string of concatenated permanent names sorted in ascending order)
    */
    uint32 GetCRC32HashOfPreregisteredSceneComponents();

    /**
        Return CRC32 hash of all registered scene components.
        (Hash of string of concatenated permanent names sorted in ascending order)
    */
    uint32 GetCRC32HashOfRegisteredSceneComponents();

    /** Return total number of registered UIComponents. */
    uint32 GetUIComponentsCount() const;

    /** Return total number of registered Scene Components. */
    uint32 GetSceneComponentsCount() const;

    /** Check if specified 'type' was registered as UIComponent. */
    bool IsRegisteredUIComponent(const Type* type) const;

    /** Check if specified 'type' was registered as Scene Component. */
    bool IsRegisteredSceneComponent(const Type* type) const;

    /**
        Return runtime id for specified component 'type'.
        The behavior is undefined until 'type' is registered in ComponentManager.
    */
    uint32 GetRuntimeComponentId(const Type* type) const;

    /**
        Return sorted (based on permanent name) runtime id for specified component 'type'.
        The behavior is undefined until 'type' is registered in ComponentManager.
    */
    uint32 GetSortedComponentId(const Type* type);

    /** 
        Return Type of Scene Component for specified component 'runtimeId'.
        Return nullptr if component with 'runtimeId' was not registered in ComponentManager.
    */
    const Type* GetSceneComponentType(uint32 runtimeId) const;

    /** Return const reference to vector of registered UIComponents types. */
    const Vector<const Type*>& GetRegisteredUIComponents() const;

    /** Return const reference to vector of registered Scene Components types. */
    const Vector<const Type*>& GetRegisteredSceneComponents() const;

private:
    ComponentManager();

    void PreregisterAllDerivedSceneComponentsRecursively();
    void CollectSceneComponentsRecursively(const Type* type, Vector<const Type*>& components);
    void UpdateSceneComponentsSortedVector(const Type* type);

    void* Uint32ToVoidPtr(uint32 value) const;
    uint32 VoidPtrToUint32(void* ptr) const;

    uint32 runtimeUIComponentsCount = 0;
    Vector<const Type*> registeredUIComponents;

    uint32 runtimeSceneComponentsCount = 0;
    Vector<const Type*> registeredSceneComponents;

    // TODO: Think of a better way for getting sorted id. There are too many containers.
    Vector<std::pair<uint32, uint32>> sceneComponentsSortedByPermanentName;

    int32 runtimeIdUserDataIndex = -1;
    int32 componentTypeUserDataIndex = -1;

    bool componentsWerePreregistered = false;
    uint32 crc32HashOfPreregisteredComponents;

    enum ComponentType : uint32
    {
        UI_COMPONENT = 1,
        SCENE_COMPONENT
    };
};
}

#include "Entity/Private/ComponentManager_impl.h"
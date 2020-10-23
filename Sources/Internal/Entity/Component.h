#pragma once

#include "Base/BaseTypes.h"
#include "Base/Serializable.h"
#include "Base/Introspection.h"
#include "Scene3D/SceneFile/SerializationContext.h"

#include "MemoryManager/MemoryProfiler.h"
#include "Reflection/Reflection.h"

/**
    \defgroup components Component
*/

namespace DAVA
{
class Entity;

class Component : public Serializable, public InspBase
{
    DAVA_ENABLE_CLASS_ALLOCATION_TRACKING(ALLOC_POOL_COMPONENT)

public:
    ~Component() override;

    const Type* GetType() const;

    /** Clone component. Then add cloned component to specified `toEntity` if `toEntity` is not nullptr. Return cloned component. */
    virtual Component* Clone(Entity* toEntity) = 0;

    void Serialize(KeyedArchive* archive, SerializationContext* serializationContext) override;
    void Deserialize(KeyedArchive* archive, SerializationContext* serializationContext) override;

    inline Entity* GetEntity() const;
    virtual void SetEntity(Entity* entity);

    /** This function should be implemented in each node that have data nodes inside it. */
    virtual void GetDataNodes(Set<DataNode*>& dataNodes);

    /** This function optimizes component before export. */
    virtual void OptimizeBeforeExport()
    {
    }

    /** Function to get data nodes of requested type to specific container you provide. */
    template <template <typename> class Container, class T>
    void GetDataNodes(Container<T>& container);

protected:
    Entity* entity = 0;
    mutable const Type* typeCache = nullptr;

    DAVA_VIRTUAL_REFLECTION(Component, InspBase);
};

} // namespace DAVA

#include "Entity/Private/Component_impl.h"

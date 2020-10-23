#pragma once

#include "Base/BaseObject.h"
#include "Base/BaseTypes.h"
#include "Base/BaseMath.h"
#include "Reflection/Reflection.h"
#include "Render/RenderBase.h"
#include "Scene3D/SceneNodeAnimationKey.h"
#include "Entity/Component.h"
#include "FileSystem/KeyedArchive.h"
#include "Scene3D/SceneFile/SerializationContext.h"
#include "Scene3D/EntityFamily.h"

#include "MemoryManager/MemoryProfiler.h"

namespace DAVA
{
class Scene;
class SceneNodeAnimationKey;
class SceneFileV2;
class DataNode;
class RenderComponent;

/**
    \brief Base class of 3D scene hierarchy. All nodes in our scene graph is inherited from this node.
 */

class Entity : public BaseObject
{
    DAVA_ENABLE_CLASS_ALLOCATION_TRACKING(ALLOC_POOL_ENTITY)
    DAVA_VIRTUAL_REFLECTION(Entity, BaseObject);

protected:
    virtual ~Entity();

public:
    Entity();

    /**
        Return scene of this entity or nullptr if this entity is not in scene.
    */
    virtual Scene* GetScene();

    //components
    void AddComponent(Component* component);

    void RemoveComponent(Component* component);
    void RemoveComponent(const Type* type, uint32 index = 0);
    template <typename T>
    void RemoveComponent(uint32 index = 0);

    void DetachComponent(Component* component);

    Component* GetComponent(const Type* type, uint32 index = 0) const;
    template <typename T>
    T* GetComponent(uint32 index = 0) const;

    Component* GetOrCreateComponent(const Type* type, uint32 index = 0);
    template <typename T>
    T* GetOrCreateComponent(uint32 index = 0);

    uint32 GetComponentCount() const;
    uint32 GetComponentCount(const Type* type) const;
    template <typename T>
    uint32 GetComponentCount() const;
    inline const ComponentMask& GetAvailableComponentMask() const;

    // working with children
    virtual void AddNode(Entity* node);
    virtual void RemoveNode(Entity* node);
    virtual void InsertBeforeNode(Entity* newNode, Entity* beforeNode);

    virtual void RemoveAllChildren();
    virtual Entity* GetNextChild(Entity* child);

    inline Entity* GetChild(int32 index) const;
    inline int32 GetChildrenCount() const;

    virtual bool FindNodesByNamePart(const String& namePart, List<Entity*>& outNodeList);

    /**
    Set entity unique ID. WARNING: Almost all time this function shouldn't be used by user, because IDs are
    generated automatically. However, it can be used in some exceptional cases, when the user exactly knows what he is doing,
    for example in the ResourceEditor during ReloadModel operation.

    Entity ID is automatically modified in this cases:
    - when entity with ID = 0 is added into scene, new ID will be generated
    - when entity with ID != 0 is added from one scene into another scene, new ID will be generated
    - cloned entity will always have ID = 0
    */
    void SetID(uint32 id);

    /**
    Return an entity ID, that is unique within scene. This ID is automatically generated, when entity (with empty ID = 0)
    is added into scene. Generated entity ID will be relative to the scene in which that entity was added.
    */
    uint32 GetID() const;

    /**
    Reset entity ID, and IDs for all child entities. ID should be reset only for entities that aren't part of the scene.
    */
    void ResetID();

    /**
    Find necessary entity in children by id. Return entity pointer or nullptr if not found.
    */
    Entity* GetEntityByID(uint32 id);

    void SetSceneID(uint32 sceneId);
    uint32 GetSceneID() const;

    /**
        \brief Find node by it's name inside this scene node.
        \param[in] name name of object you want to find. 
        \returns pointer to the object if object with such name available, 0 in case if it's not exists.
     */
    virtual Entity* FindByName(const FastName& name);

    /**
        \brief Find node by it's name inside this scene node.
        \param[in] name name of object you want to find. 
        \returns pointer to the object if object with such name available, 0 in case if it's not exists.
     */
    virtual Entity* FindByName(const char* name);
    /**
        \brief Set name of this particular node.
        \param[in] new name for this node
     */
    virtual void SetName(const FastName& name);

    /**
        \brief Set name of this particular node.
        \param[in] new name for this node
     */
    virtual void SetName(const char* name);

    /**
        \brief Get name of this particular node.
        \returns name of this node
     */
    inline const FastName& GetName() const;

    /**
        \brief Get full name of this node from root. This function is slow because it go up by hierarchy and make full node name.
        \returns this node full name from root. Example [MaxScene->camera->instance0]
     */
    String GetFullName();

    Entity* GetParent();

    Matrix4 AccamulateLocalTransform(Entity* fromParent); //TODO: delete it
    Matrix4 AccamulateTransformUptoFarParent(Entity* farParent); //TODO: delete it
    /*
    \brief Go down by hierarchy and bake all transforms.
    Function can be used to bake transforms to minimize amount of matrix multiplications.
    */
    virtual void BakeTransforms();

    void SetVisible(const bool& isVisible);
    bool GetVisible();

    enum EntityFlags
    {
        NODE_VISIBLE = 1 << 1, // is node and subnodes should draw

        TRANSFORM_NEED_UPDATE = 1 << 11,
        TRANSFORM_DIRTY = 1 << 12
    };

    inline void AddFlag(int32 flagToAdd);
    inline void RemoveFlag(int32 flagToRemove);
    inline uint32 GetFlags() const;
    void AddFlagRecursive(int32 flagToAdd);
    void RemoveFlagRecursive(int32 flagToRemove);

    virtual Entity* Clone(Entity* dstNode = NULL);

    /**
        \brief function to enable or disable debug drawing for particular node.
        By default it's not recursive. Some objects may support flags only partially.
        For example if node do not have bounding box flag DEBUG_DRAW_AABBOX will not produce any output
        These flags are mostly for debug purposes and we do not guarantee that logic of the debug rendering will remain unchanged between 
        framework versions.
     
        \param[in] debugFlags flags to be set
        \param[in] isRecursive do you want to set flags recursively
     
     */
    void SetDebugFlags(uint32 debugFlags, bool isRecursive = false);
    /**
        \brief function returns debug flags of specific node
        \returns flags of this specific scene node
     */
    uint32 GetDebugFlags() const;

    void SetSolid(bool isSolid);
    bool GetSolid() const;

    void SetLocked(bool isLocked);
    bool GetLocked() const;

    void SetNotRemovable(bool notRemovabe);
    bool GetNotRemovable() const;

    /**
        \brief function returns maximum bounding box of scene in world coordinates.
        \returns bounding box
     */
    virtual AABBox3 GetWTMaximumBoundingBoxSlow(); //TODO: delete it

    /**
        \brief virtual function to save node to KeyedArchive
     */
    virtual void Save(KeyedArchive* archive, SerializationContext* serializationContext);

    /**
        \brief virtual function to load node to KeyedArchive
     */
    virtual void Load(KeyedArchive* archive, SerializationContext* serializationContext);

    /**
        \brief This function should be implemented in each node that have data nodes inside it.
     */
    virtual void GetDataNodes(Set<DataNode*>& dataNodes);
    /**
        \brief Function to get data nodes of requested type to specific container you provide.
     */
    template <template <typename, typename> class Container, class T, class A>
    void GetDataNodes(Container<T, A>& container);
    /**
	 \brief Optimize scene before export.
     */
    void OptimizeBeforeExport();

    /**
        \brief Function to get child nodes of requested type and move them to specific container you provide.
        For example if you want to get a list of MeshInstanceNodes you should do the following.
        \code   
        #include "Scene3D/Entity.h"
        #include "Scene3D/MeshInstanceNode.h"  // You should include MeshInstanceNode because Entity class do not know the type of node you want to convert to. 
        
        void YourClass::YourFunction()
        {
            List<MeshInstanceNode*> meshNodes;
            scene->GetChildNodes(meshNodes);
        }
        \endcode
     */
    template <template <typename, typename> class Container, class T, class A>
    void GetChildNodes(Container<T, A>& container);

    /**
        Puts into `container` all child entities
        that have component of given `type`.
        Function may work `recursively`
    */
    template <template <typename, typename> class Container, class A>
    void GetChildEntitiesWithComponent(Container<Entity*, A>& container, const Type* type, bool recursively = true);

    /**
        Puts into `container` all child entities
        that have component of given `type`.
        Function may work `recursively`
    */
    template <template <typename, typename> class Container, class A>
    void GetChildEntitiesWithComponent(Container<const Entity*, A>& container, const Type* type, bool recursively = true) const;

    /**
        Puts into `container` recursively all child entities
        that satisfy given unary predicate `pred`
    */
    template <template <typename, typename> class Container, class A, class Pred>
    void GetChildEntitiesWithCondition(Container<Entity*, A>& container, Pred pred);

    uint32 CountChildEntitiesWithComponent(const Type* type, bool recursive = false) const;

    /**
        \brief This function is called after scene is loaded from file.
        You can perform additional initialization here.
     */
    virtual void SceneDidLoaded();

    // Property names.
    static const char* SCENE_NODE_IS_SOLID_PROPERTY_NAME;
    static const char* SCENE_NODE_IS_LOCKED_PROPERTY_NAME;
    static const char* SCENE_NODE_IS_NOT_REMOVABLE_PROPERTY_NAME;

    static FastName EntityNameFieldName;
    static const char* componentFieldString;

    void FindComponentsByTypeRecursive(const Type* type, List<DAVA::Entity*>& components);

    Vector<Entity*> children;

    EntityFamily* GetFamily() const;

protected:
    void UpdateFamily();
    void RemoveAllComponents();
    void LoadComponentsV7(KeyedArchive* compsArch, SerializationContext* serializationContext);

    String RecursiveBuildFullName(Entity* node, Entity* endNode);

    void SetParent(Entity* node);

    Scene* scene = nullptr;
    Entity* parent = nullptr;
    FastName name;
    uint32 flags = NODE_VISIBLE;
    uint32 id = 0;
    uint32 sceneId = 0;

    /**
    \brief Function to set scene for node and it's children.
    Function goes recursively and set scene for this node, and each child.
    \param[in] _scene pointer to scene we want to set as holder for this node.
    */
    virtual void SetScene(Scene* _scene); //TODO: Move SetScene to private

private:
    Vector<Component*> components;
    EntityFamily* family = nullptr;
    void DetachComponent(Vector<Component*>::iterator& it);
    void RemoveComponent(Vector<Component*>::iterator& it);

    friend class Scene;
    friend class SceneFileV2;
};

inline uint32 Entity::GetID() const
{
    return id;
}

inline void Entity::SetID(uint32 id_)
{
    id = id_;
}

inline void Entity::ResetID()
{
    DVASSERT(nullptr == GetScene() && "ID can safely be reset in entities that aren't part of scene");

    id = 0;
    sceneId = 0;
    for (auto child : children)
    {
        child->ResetID();
    }
}

inline void Entity::AddFlag(int32 flagToAdd)
{
    flags |= flagToAdd;
}

inline void Entity::RemoveFlag(int32 flagToRemove)
{
    flags &= ~flagToRemove;
}

inline uint32 Entity::GetFlags() const
{
    return flags;
}

inline Entity* Entity::GetParent()
{
    return parent;
}

inline const FastName& Entity::GetName() const
{
    return name;
}

template <template <typename, typename> class Container, class T, class A>
void Entity::GetDataNodes(Container<T, A>& container)
{
    Set<DataNode*> objects;
    GetDataNodes(objects);

    Set<DataNode*>::const_iterator end = objects.end();
    for (Set<DataNode*>::iterator t = objects.begin(); t != end; ++t)
    {
        DataNode* obj = *t;

        T res = dynamic_cast<T>(obj);
        if (res != nullptr)
        {
            container.push_back(res);
        }
    }
}

template <template <typename, typename> class Container, class T, class A>
void Entity::GetChildNodes(Container<T, A>& container)
{
    Vector<Entity*>::const_iterator end = children.end();
    for (Vector<Entity*>::iterator t = children.begin(); t != end; ++t)
    {
        Entity* obj = *t;

        T res = dynamic_cast<T>(obj);
        if (res != nullptr)
        {
            container.push_back(res);
        }

        obj->GetChildNodes(container);
    }
}

template <template <typename, typename> class Container, class A>
void Entity::GetChildEntitiesWithComponent(Container<Entity*, A>& container, const Type* type, bool recursively)
{
    for (auto& child : children)
    {
        if (child->GetComponentCount(type) > 0)
        {
            container.push_back(child);
        }

        if (recursively)
        {
            child->GetChildEntitiesWithComponent(container, type, recursively);
        }
    }
}

template <template <typename, typename> class Container, class A>
void Entity::GetChildEntitiesWithComponent(Container<const Entity*, A>& container, const Type* type, bool recursively) const
{
    for (auto& child : children)
    {
        if (child->GetComponentCount(type) > 0)
        {
            container.push_back(child);
        }

        if (recursively)
        {
            child->GetChildEntitiesWithComponent(container, type, recursively);
        }
    }
}

template <template <typename, typename> class Container, class A, class Pred>
void Entity::GetChildEntitiesWithCondition(Container<Entity*, A>& container, Pred pred)
{
    for (auto& child : children)
    {
        if (pred(child) == true)
        {
            container.push_back(child);
        }

        child->GetChildEntitiesWithCondition(container, pred);
    }
}

inline const ComponentMask& Entity::GetAvailableComponentMask() const
{
    return family->GetComponentsMask();
}

inline Entity* Entity::GetChild(int32 index) const
{
    return children[index];
}

inline int32 Entity::GetChildrenCount() const
{
    return static_cast<int32>(children.size());
}

inline uint32 Entity::GetComponentCount() const
{
    return static_cast<uint32>(components.size());
}

inline void Entity::UpdateFamily()
{
    EntityFamily::Release(family);
    family = EntityFamily::GetOrCreate(components);
}

inline void Entity::RemoveAllComponents()
{
    while (!components.empty())
    {
        RemoveComponent(--components.end());
    }
}

inline void Entity::RemoveComponent(const Type* type, uint32 index)
{
    Component* c = GetComponent(type, index);
    if (c)
    {
        RemoveComponent(c);
    }
}

inline void Entity::RemoveComponent(Component* component)
{
    DetachComponent(component);
    SafeDelete(component);
}

template <typename T>
inline void Entity::RemoveComponent(uint32 index)
{
    RemoveComponent(Type::Instance<T>(), index);
}

template <typename T>
inline T* Entity::GetComponent(uint32 index) const
{
    return DynamicTypeCheck<T*>(GetComponent(Type::Instance<T>(), index));
}

template <typename T>
inline T* Entity::GetOrCreateComponent(uint32 index)
{
    return DynamicTypeCheck<T*>(GetOrCreateComponent(Type::Instance<T>(), index));
}

inline void Entity::DetachComponent(Component* component)
{
    DVASSERT(component);

    auto it = std::find(components.begin(), components.end(), component);
    DetachComponent(it);
}

inline Scene* Entity::GetScene()
{
    return scene;
}

inline uint32 Entity::GetComponentCount(const Type* type) const
{
    return family->GetComponentsCount(type);
}

template <typename T>
inline uint32 Entity::GetComponentCount() const
{
    return GetComponentCount(Type::Instance<T>());
}

inline bool Entity::GetVisible()
{
    return (flags & NODE_VISIBLE) != 0;
}

inline uint32 Entity::GetSceneID() const
{
    return sceneId;
}

inline void Entity::SetSceneID(uint32 sceneId_)
{
    sceneId = sceneId_;
}

inline EntityFamily* Entity::GetFamily() const
{
    return family;
}

} // namespace DAVA

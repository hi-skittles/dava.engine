#pragma once

#include "Base/FastName.h"
#include "Base/RefPtr.h"
#include "Entity/SceneSystem.h"
#include "FileSystem/FilePath.h"
#include "FileSystem/KeyedArchive.h"
#include "Functional/Function.h"

#include <memory>

namespace DAVA
{
class SlotComponent;

/**
    \ingroup systems
    \brief SlotSystem manage slots (SlotComponent), loaded items and provide methods to load item into slot or into all
    slots with some name. SlotSystem delegate item loading to ExternalEntityLoader, that can implement some custom logic.
    For example, hold cache of Entities or add some properties to loaded Entity. By default ExternalEntityLoader implement
    async loaded strategy. Method AttachItemToSlot will return 'empty Entity' immediately. Loading will process in separate
    thread. Item will be attached into 'empty Entity' after it will be fully loaded.

    SlotSystem use ItemsCache to hold result of config files parsing. By default SlotSystem use one ItemsCache per scene,
    but game can override this behaviour and call SetSharedCache to change scope of this cache.

    Config parsing is a lazy operation. In method AttachItemToSlot SlotSystem call ItemsCache to find item with some name.
    If cache has not parse config file of SlotComponent yet, it will parse this config.
*/
class SlotSystem final : public SceneSystem
{
public:
    class ItemsCache
    {
    public:
        struct Item
        {
            FastName itemName;
            FastName type;
            FilePath scenePath;
            RefPtr<KeyedArchive> additionalParams;
        };

        void LoadConfigFile(const FilePath& configPath);
        const Item* LookUpItem(const FilePath& configPath, FastName itemName);
        Vector<Item> GetItems(const FilePath& configPath);
        void InvalidateConfig(const FilePath& configPath);
        bool IsConfigParsed(const FilePath& configPath) const;

    private:
        void LoadYamlConfig(const FilePath& configPath);
        void LoadXmlConfig(const FilePath& configPath);
        class XmlConfigParser;
        struct ItemLess
        {
            bool operator()(const Item& item1, const Item& item2) const;
        };

        UnorderedMap<String, Set<Item, ItemLess>> cachedItems;
    };

    /** \brief Class that SlotSystem delegates loading of item */
    class ExternalEntityLoader : public std::enable_shared_from_this<ExternalEntityLoader>
    {
    public:
        virtual ~ExternalEntityLoader() = default;
        /**
            Load item with path \c path, attach it into entity \c rootEntity and call \c finishCallback.
            Loading can be processed in worker thread, but finishCallback should be called from 'Process' method.
            If argument of finishCallback is empty, slot system think that loading was successful, else slot system set
            state for slot as LOADING_FAILED and print argument into Logger::Error
        */
        virtual void Load(RefPtr<Entity> rootEntity, const FilePath& path, const Function<void(String&&)>& finishCallback) = 0;
        /** Add root entity of slot item info scene hierarchy. Default implementation simple call parent->AddNode(child) */
        virtual void AddEntity(Entity* parent, Entity* child);
        /** Slot system call this method from every SlotSystem::Process */
        virtual void Process(float32 delta) = 0;

    protected:
        /** Called before ExternalEntityLoader will be detached from slot system */
        virtual void Reset() = 0;
        Scene* scene = 0;

    private:
        friend class SlotSystem;
        void SetScene(Scene* scene);
    };

    SlotSystem(Scene* scene);
    ~SlotSystem();

    /**
        Override default ItemsCache. Changing of items cache will produce cache invalidation,
        but will not produce unloading of loaded items 
    */
    void SetSharedCache(std::shared_ptr<ItemsCache> cache);
    /**
        Get result of parsing config file. This method can be used to precache config files
        \return Items that described in \c configPath
    */
    Vector<ItemsCache::Item> GetItems(const FilePath& configPath);
    void InvalidateConfig(const FilePath& configPath);
    bool IsConfigParsed(const FilePath& configPath) const;
    static Vector<ItemsCache::Item> ParseConfig(const FilePath& configPath);
    /** Override default async loader */
    void SetExternalEntityLoader(std::shared_ptr<ExternalEntityLoader> externalEntityLoader);

    void UnregisterEntity(Entity* entity) override;

    void AddEntity(Entity* entity) override;
    void RemoveEntity(Entity* entity) override;

    void AddComponent(Entity* entity, Component* component) override;
    void RemoveComponent(Entity* entity, Component* component) override;

    void PrepareForRemove() override;
    void Process(float32 timeElapsed) override;

    /**
        Load and attach item with name \c itemName into all slots with name \c slotName recursively in hierarchy with root \c rootEntity
        \arg \c rootEntity root of hierarchy. Should be attached to scene
    */
    void AttachItemToSlot(Entity* rootEntity, FastName slotName, FastName itemName);
    /**
        Load and attach item with name \c itemName into slots \c component
        \return Loaded and attached to slot Entity
    */
    RefPtr<Entity> AttachItemToSlot(SlotComponent* component, FastName itemName);
    /** Attach \c entity into slot \c component and named it by name equal to \c itemName */
    void AttachEntityToSlot(SlotComponent* component, Entity* entity, FastName itemName);

    /** Lookup entity that currently loaded into slot \c component. Can return nullptr if slot currently empty */
    Entity* LookUpLoadedEntity(SlotComponent* component) const;
    /** Lookup slot by \c entity that currently loaded into it. Can return nullptr if \c entity is not loaded item*/
    SlotComponent* LookUpSlot(Entity* entity) const;
    /** Helper function to get local transform of joint that \c component attached to. If \c component doesn't attached to joint, return Identity*/
    Matrix4 GetJointTransform(SlotComponent* component) const;
    /** Helper function to get local transform entity that attached to this \c component.*/
    Matrix4 GetResultTranform(SlotComponent* component) const;

    /** Set attachment transform for slot \c component*/
    void SetAttachmentTransform(SlotComponent* component, const Matrix4& transform);

    /** Describe different state of slot */
    enum class eSlotState
    {
        /** Slot is empty and the was not trying to load item into this slot */
        NOT_LOADED,
        /** Item for slot is currently is loading */
        LOADING,
        /** Item is completely loaded into slot */
        LOADED,
        /** Last loading of item for slot was failed */
        LOADING_FAILED
    };

    /** Get current state of slot */
    eSlotState GetSlotState(const SlotComponent* component) const;

protected:
    void SetScene(Scene* scene) override;

private:
    void AttachEntityToSlotImpl(SlotComponent* component, Entity* entity, FastName itemName, SlotSystem::eSlotState state);
    void UnloadItem(SlotComponent* component);

    uint32 GetComponentIndex(const SlotComponent* component) const;

    struct SlotNode
    {
        static const int32 STATE_MASK = 0xFF;
        static const int32 FLAGS_MASK = 0xFF00;

        enum eFlags : int32
        {
            ATTACHMENT_TRANSFORM_CHANGED = 0x100
        };

        SlotComponent* component = nullptr;
        Entity* loadedEnity = nullptr;
        int32 flags = 0;
    };

    Vector<SlotNode> nodes;
    uint32 loadedItemsCount = 0;

    void SetState(SlotNode& node, eSlotState state);
    eSlotState GetState(const SlotNode& node) const;
    bool TestFlag(SlotNode& node, int32 flag) const;
    void RaiseFlag(SlotNode& node, int32 flag);
    void ResetFlag(SlotNode& node, int32 flag);

    std::shared_ptr<ExternalEntityLoader> externalEntityLoader;
    std::shared_ptr<ItemsCache> sharedCache;
};

} // namespace DAVA

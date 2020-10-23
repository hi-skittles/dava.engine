#include "Scene3D/Systems/SlotSystem.h"
#include "Scene3D/Scene.h"
#include "Scene3D/Components/SlotComponent.h"
#include "Scene3D/Components/ComponentHelpers.h"
#include "Scene3D/Components/TransformComponent.h"
#include "Scene3D/Components/SkeletonComponent.h"
#include "Scene3D/Systems/Private/AsyncSlotExternalLoader.h"
#include "Math/Transform.h"
#include "Math/TransformUtils.h"
#include "FileSystem/YamlParser.h"
#include "FileSystem/XMLParser.h"
#include "FileSystem/XMLParserStatus.h"
#include "FileSystem/XMLParserDelegate.h"
#include "FileSystem/YamlNode.h"
#include "Logger/Logger.h"

namespace DAVA
{
class SlotSystem::ItemsCache::XmlConfigParser : public XMLParserDelegate
{
public:
    XmlConfigParser(Set<SlotSystem::ItemsCache::Item, SlotSystem::ItemsCache::ItemLess>* items);

    void OnElementStarted(const String& elementName, const String& namespaceURI, const String& qualifedName, const Map<String, String>& attributes) override;
    void OnElementEnded(const String& elementName, const String& namespaceURI, const String& qualifedName) override;
    void OnFoundCharacters(const String& chars) override;

    bool IsErrorFound() const;
    bool IsDublicatesFound() const;

private:
    Set<Item, ItemLess>* items;
    bool errorFound = false;
    bool duplicatesFound = false;
};

SlotSystem::ItemsCache::XmlConfigParser::XmlConfigParser(Set<Item, ItemLess>* items_)
    : items(items_)
{
}

void SlotSystem::ItemsCache::XmlConfigParser::OnElementStarted(const String& elementName, const String& namespaceURI, const String& qualifedName, const Map<String, String>& attributes)
{
    String nameKey("Name");
    String typeKey("Type");
    String pathKey("Path");

    if (elementName == "item")
    {
        Item item;
        for (const auto& attributeNode : attributes)
        {
            const String& key = attributeNode.first;
            const String& value = attributeNode.second;
            if (key == nameKey)
            {
                item.itemName = FastName(value);
            }
            else if (key == typeKey)
            {
                item.type = FastName(value);
            }
            else if (key == pathKey)
            {
                item.scenePath = FilePath(value);
            }
            else
            {
                if (item.additionalParams.Get() == nullptr)
                {
                    item.additionalParams.ConstructInplace();
                }

                item.additionalParams->SetString(key, value);
            }
        }

        if (item.itemName.IsValid() == false || item.scenePath.IsEmpty() == true)
        {
            errorFound = true;
        }

        duplicatesFound |= (items->insert(item).second == false);
    }
}

void SlotSystem::ItemsCache::XmlConfigParser::OnElementEnded(const String& elementName, const String& namespaceURI, const String& qualifedName)
{
}

void SlotSystem::ItemsCache::XmlConfigParser::OnFoundCharacters(const String& chars)
{
}

bool SlotSystem::ItemsCache::XmlConfigParser::IsErrorFound() const
{
    return errorFound;
}

bool SlotSystem::ItemsCache::XmlConfigParser::IsDublicatesFound() const
{
    return duplicatesFound;
}

void SlotSystem::ItemsCache::LoadConfigFile(const FilePath& configPath)
{
    String extension = configPath.GetExtension();
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
    if (extension == ".yaml")
    {
        LoadYamlConfig(configPath);
    }
    else if (extension == ".xml")
    {
        LoadXmlConfig(configPath);
    }
    else
    {
        Logger::Error("Unknown slot config file extension %s", configPath.GetAbsolutePathname().c_str());
    }
}

void SlotSystem::ItemsCache::LoadYamlConfig(const FilePath& configPath)
{
    RefPtr<YamlParser> parser(YamlParser::Create(configPath));
    if (!parser)
    {
        Logger::Error("Couldn't parse yaml file %s", configPath.GetAbsolutePathname().c_str());
        return;
    }

    YamlNode* rootNode = parser->GetRootNode();
    if (rootNode == nullptr)
    {
        Logger::Error("Configuration file %s is empty", configPath.GetAbsolutePathname().c_str());
        return;
    }

    if (rootNode->GetType() != YamlNode::eType::TYPE_ARRAY)
    {
        Logger::Error("Incorrect file format: %s", configPath.GetAbsolutePathname().c_str());
        return;
    }

    String nameKey("Name");
    String typeKey("Type");
    String pathKey("Path");

    bool incorrectItemsFound = false;
    bool duplicatesFound = false;
    Set<Item, ItemLess>& items = cachedItems[configPath.GetAbsolutePathname()];

    const auto& yamlNodes = rootNode->AsVector();
    size_t propertiesCount = yamlNodes.size();
    for (const RefPtr<YamlNode>& currentNode : yamlNodes)
    {
        uint32 fieldsCount = currentNode->GetCount();

        Item newItem;
        for (uint32 fieldIndex = 0; fieldIndex < fieldsCount; ++fieldIndex)
        {
            const YamlNode* fieldNode = currentNode->Get(fieldIndex);
            const String& key = currentNode->GetItemKeyName(fieldIndex);
            if (fieldNode->GetType() == YamlNode::TYPE_STRING)
            {
                if (key == nameKey && fieldNode->GetType() == YamlNode::TYPE_STRING)
                {
                    newItem.itemName = FastName(fieldNode->AsString());
                }
                else if (key == pathKey)
                {
                    String path = fieldNode->AsString();
                    newItem.scenePath = FilePath(path);
                }
                else if (key == typeKey)
                {
                    newItem.type = FastName(fieldNode->AsString());
                }
                else
                {
                    if (nullptr == newItem.additionalParams)
                    {
                        newItem.additionalParams.ConstructInplace();
                    }

                    newItem.additionalParams->SetString(key, fieldNode->AsString());
                }
            }
        }

        bool isItemValid = newItem.itemName.IsValid() && newItem.scenePath.IsEmpty() == false;
        if (isItemValid == false)
        {
            incorrectItemsFound = true;
        }
        else
        {
            duplicatesFound |= (items.insert(newItem).second == false);
        }
    }

    if (incorrectItemsFound == true)
    {
        Logger::Error("Yaml parsing error. Config file %s contains incomplete items", configPath.GetAbsolutePathname().c_str());
    }

    if (duplicatesFound == true)
    {
        Logger::Error("Yaml parsing error. Config file %s contains duplicated items", configPath.GetAbsolutePathname().c_str());
    }

    if (items.empty() == true)
    {
        Logger::Error("Yaml parsing error. No one item was found. File format probably incorrect %s", configPath.GetAbsolutePathname().c_str());
    }
}

void SlotSystem::ItemsCache::LoadXmlConfig(const FilePath& configPath)
{
    Set<Item, ItemLess>& items = cachedItems[configPath.GetAbsolutePathname()];
    ItemsCache::XmlConfigParser parser(&items);
    XMLParserStatus status = XMLParser::ParseFileEx(configPath, &parser);
    if (status.Success())
    {
        if (items.empty() == true)
        {
            Logger::Error("XML parsing error. No one item was found. File format probably incorrect %s", configPath.GetAbsolutePathname().c_str());
        }
    }
    else
    {
        Logger::Error("XML parsing error in file `%s`: %s (%d:%d)", configPath.GetAbsolutePathname().c_str(), status.errorMessage.c_str(), status.errorLine, status.errorPosition);
    }
}

const SlotSystem::ItemsCache::Item* SlotSystem::ItemsCache::LookUpItem(const FilePath& configPath, FastName itemName)
{
    String absolutePath = configPath.GetAbsolutePathname();
    auto configIter = cachedItems.find(absolutePath);
    if (configIter == cachedItems.end())
    {
        LoadConfigFile(configPath);
        configIter = cachedItems.find(absolutePath);
    }

    if (configIter == cachedItems.end())
    {
        return nullptr;
    }

    Item key;
    key.itemName = itemName;

    auto itemIter = configIter->second.find(key);
    if (itemIter == configIter->second.end())
    {
        return nullptr;
    }

    return &(*itemIter);
}

Vector<SlotSystem::ItemsCache::Item> SlotSystem::ItemsCache::GetItems(const FilePath& configPath)
{
    Vector<Item> result;
    if (configPath.IsEmpty())
    {
        return result;
    }

    String absolutePath = configPath.GetAbsolutePathname();
    auto configIter = cachedItems.find(absolutePath);
    if (configIter == cachedItems.end())
    {
        LoadConfigFile(configPath);
        configIter = cachedItems.find(absolutePath);
    }

    if (configIter != cachedItems.end())
    {
        std::copy(configIter->second.begin(), configIter->second.end(), std::back_inserter(result));
    }

    return result;
}

void SlotSystem::ItemsCache::InvalidateConfig(const FilePath& configPath)
{
    if (configPath.IsEmpty())
    {
        return;
    }

    String absolutePath = configPath.GetAbsolutePathname();
    auto iter = cachedItems.find(absolutePath);
    if (iter != cachedItems.end())
    {
        cachedItems.erase(iter);
    }
}

bool SlotSystem::ItemsCache::IsConfigParsed(const FilePath& configPath) const
{
    String absolutePath = configPath.GetAbsolutePathname();
    auto iter = cachedItems.find(absolutePath);
    return iter != cachedItems.end();
}

bool SlotSystem::ItemsCache::ItemLess::operator()(const Item& item1, const Item& item2) const
{
    return item1.itemName < item2.itemName;
}

void SlotSystem::ExternalEntityLoader::SetScene(Scene* scene)
{
    if (scene == nullptr)
    {
        Reset();
    }
    this->scene = scene;
}

void SlotSystem::ExternalEntityLoader::AddEntity(Entity* parent, Entity* child)
{
    parent->AddNode(child);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                              SlotSystem                                                      //
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////

SlotSystem::SlotSystem(Scene* scene)
    : SceneSystem(scene)
    , externalEntityLoader(new AsyncSlotExternalLoader())
    , sharedCache(new ItemsCache())
{
}

SlotSystem::~SlotSystem()
{
    DVASSERT(externalEntityLoader != nullptr);
    externalEntityLoader->Reset();
}

void SlotSystem::SetSharedCache(std::shared_ptr<ItemsCache> cache)
{
    sharedCache = cache;
}

Vector<SlotSystem::ItemsCache::Item> SlotSystem::GetItems(const FilePath& configPath)
{
    return sharedCache->GetItems(configPath);
}

void SlotSystem::InvalidateConfig(const FilePath& configPath)
{
    sharedCache->InvalidateConfig(configPath);
}

bool SlotSystem::IsConfigParsed(const FilePath& configPath) const
{
    return sharedCache->IsConfigParsed(configPath);
}

DAVA::Vector<SlotSystem::ItemsCache::Item> SlotSystem::ParseConfig(const FilePath& configPath)
{
    return ItemsCache().GetItems(configPath);
}

void SlotSystem::SetExternalEntityLoader(std::shared_ptr<ExternalEntityLoader> externalEntityLoader_)
{
    DVASSERT(externalEntityLoader_ != nullptr);
    DVASSERT(externalEntityLoader != nullptr);
    externalEntityLoader->SetScene(nullptr);
    externalEntityLoader = externalEntityLoader_;
    externalEntityLoader->SetScene(GetScene());
}

void SlotSystem::UnregisterEntity(Entity* entity)
{
    if (loadedItemsCount > 0)
    {
        auto loadedIter = std::find_if(nodes.begin(), nodes.end(), [entity](const SlotNode& node) {
            return node.loadedEnity == entity;
        });

        uint32 index = static_cast<uint32>(std::distance(nodes.begin(), loadedIter));
        if (index < loadedItemsCount)
        {
            SlotNode& node = nodes[index];
            SetState(node, eSlotState::NOT_LOADED);
            ResetFlag(node, SlotNode::ATTACHMENT_TRANSFORM_CHANGED);
            node.loadedEnity = nullptr;
            std::swap(nodes[index], nodes[loadedItemsCount - 1]);
            --loadedItemsCount;
        }
    }

    SceneSystem::UnregisterEntity(entity);
}

void SlotSystem::AddEntity(Entity* entity)
{
    uint32 count = entity->GetComponentCount<SlotComponent>();
    for (uint32 i = 0; i < count; ++i)
    {
        AddComponent(entity, entity->GetComponent<SlotComponent>(i));
    }
}

void SlotSystem::RemoveEntity(Entity* entity)
{
    uint32 count = entity->GetComponentCount<SlotComponent>();
    for (uint32 i = 0; i < count; ++i)
    {
        RemoveComponent(entity, entity->GetComponent<SlotComponent>(i));
    }
}

void SlotSystem::AddComponent(Entity* entity, Component* component)
{
    DVASSERT(component->GetType()->Is<SlotComponent>());
    SlotNode node;
    node.component = static_cast<SlotComponent*>(component);
    SetState(node, eSlotState::NOT_LOADED);
    nodes.push_back(node);
}

void SlotSystem::RemoveComponent(Entity* entity, Component* component)
{
    DVASSERT(component->GetType()->Is<SlotComponent>());
    UnloadItem(static_cast<SlotComponent*>(component));
    uint32 index = GetComponentIndex(static_cast<SlotComponent*>(component));
    RemoveExchangingWithLast(nodes, index);
}

void SlotSystem::PrepareForRemove()
{
    nodes.clear();
}

void SlotSystem::Process(float32 timeElapsed)
{
    for (uint32 i = 0; i < loadedItemsCount; ++i)
    {
        SlotNode& node = nodes[i];

        TransformComponent* transform = node.loadedEnity->GetComponent<TransformComponent>();
        if (node.component->GetJointUID().IsValid())
        {
            transform->SetLocalMatrix(GetResultTranform(node.component));
            ResetFlag(node, SlotNode::ATTACHMENT_TRANSFORM_CHANGED);
        }
        else if (TestFlag(node, SlotNode::ATTACHMENT_TRANSFORM_CHANGED))
        {
            transform->SetLocalTransform(node.component->GetAttachmentTransform());
            ResetFlag(node, SlotNode::ATTACHMENT_TRANSFORM_CHANGED);
        }
    }

    DVASSERT(externalEntityLoader != nullptr);
    externalEntityLoader->Process(timeElapsed);
}

void SlotSystem::AttachItemToSlot(Entity* rootEntity, FastName slotName, FastName itemName)
{
    uint32 slotsCount = rootEntity->GetComponentCount<SlotComponent>();
    for (uint32 i = 0; i < slotsCount; ++i)
    {
        SlotComponent* slotComponent = rootEntity->GetComponent<SlotComponent>(i);
        if (slotComponent->GetSlotName() == slotName)
        {
            AttachItemToSlot(slotComponent, itemName);
        }
    }

    uint32 childCount = rootEntity->GetChildrenCount();
    for (uint32 childIndex = 0; childIndex < childCount; ++childIndex)
    {
        AttachItemToSlot(rootEntity->GetChild(childIndex), slotName, itemName);
    }
}

RefPtr<Entity> SlotSystem::AttachItemToSlot(SlotComponent* component, FastName itemName)
{
    UnloadItem(component);

    const FilePath& configPath = component->GetConfigFilePath();
    uint32 filtersCount = component->GetTypeFiltersCount();

    const ItemsCache::Item* item = sharedCache->LookUpItem(configPath, itemName);
    if (item == nullptr)
    {
        Logger::Error("Couldn't find item %s in config file %s for slot %s: ", itemName.c_str(),
                      component->GetConfigFilePath().GetAbsolutePathname().c_str(),
                      component->GetSlotName().c_str());
        uint32 index = GetComponentIndex(component);
        SetState(nodes[index], eSlotState::LOADING_FAILED);
        return RefPtr<Entity>();
    }

    RefPtr<Entity> slotRootEntity(new Entity());
    DVASSERT(externalEntityLoader != nullptr);
    externalEntityLoader->Load(slotRootEntity, item->scenePath.GetAbsolutePathname(), [this, component, itemName](String&& message) {
        auto iter = std::find_if(nodes.begin(), nodes.end(), [component](const SlotNode& node) {
            return node.component == component;
        });

        if (iter == nodes.end())
        {
            return;
        }

        // if iter->loadedEnity == nullptr, root for loaded item has been already remove from scene
        if (iter->loadedEnity != nullptr)
        {
            if (message.empty() == false)
            {
                // "Component" was found in "components". This means that component still in system and pointer is valid
                Logger::Error("Loading item %s to slot %s failed: %s", itemName.c_str(), component->GetSlotName().c_str(), message.c_str());
                SetState(*iter, eSlotState::LOADING_FAILED);
            }
            else
            {
                SetState(*iter, eSlotState::LOADED);
            }
        }
    });
    externalEntityLoader->AddEntity(component->GetEntity(), slotRootEntity.Get());
    AttachEntityToSlotImpl(component, slotRootEntity.Get(), item->itemName, eSlotState::LOADING);
    return slotRootEntity;
}

void SlotSystem::AttachEntityToSlot(SlotComponent* component, Entity* entity, FastName itemName)
{
    UnloadItem(component);
    DVASSERT(component->GetEntity() != nullptr);
    DVASSERT(component->GetEntity()->GetScene() == GetScene());

    Entity* parentEntity = component->GetEntity();
    externalEntityLoader->AddEntity(parentEntity, entity);
    AttachEntityToSlotImpl(component, entity, itemName, eSlotState::LOADED);
}

Entity* SlotSystem::LookUpLoadedEntity(SlotComponent* component) const
{
    if (component->GetEntity() == nullptr)
    {
        return nullptr;
    }

    uint32 index = GetComponentIndex(component);
    return nodes[index].loadedEnity;
}

SlotComponent* SlotSystem::LookUpSlot(Entity* entity) const
{
    auto iter = std::find_if(nodes.begin(), nodes.end(), [entity](const SlotNode& node) {
        return node.loadedEnity == entity;
    });

    if (iter != nodes.end())
    {
        return iter->component;
    }

    return nullptr;
}

Matrix4 SlotSystem::GetJointTransform(SlotComponent* component) const
{
    DVASSERT(component->GetEntity()->GetScene() == GetScene());
    Matrix4 jointTransform;
    FastName boneName = component->GetJointUID();
    if (boneName.IsValid())
    {
        SkeletonComponent* skeleton = GetSkeletonComponent(component->GetEntity());
        DVASSERT(skeleton != nullptr);

        if (component->attachmentToJointIndex == SkeletonComponent::INVALID_JOINT_INDEX)
            component->attachmentToJointIndex = skeleton->GetJointIndex(boneName);

        uint32 jointIndex = component->attachmentToJointIndex;
        DVASSERT(jointIndex != SkeletonComponent::INVALID_JOINT_INDEX);

        const JointTransform& transform = skeleton->GetJointObjectSpaceTransform(jointIndex);
        jointTransform = transform.GetMatrix();
    }

    return jointTransform;
}

DAVA::Matrix4 SlotSystem::GetResultTranform(SlotComponent* component) const
{
    DVASSERT(component->GetEntity()->GetScene() == GetScene());
    FastName boneName = component->GetJointUID();
    DVASSERT(boneName.IsValid());
    SkeletonComponent* skeleton = GetSkeletonComponent(component->GetEntity());
    DVASSERT(skeleton != nullptr);

    if (component->attachmentToJointIndex == SkeletonComponent::INVALID_JOINT_INDEX)
        component->attachmentToJointIndex = skeleton->GetJointIndex(boneName);

    uint32 jointIndex = component->attachmentToJointIndex;
    DVASSERT(jointIndex != SkeletonComponent::INVALID_JOINT_INDEX);

    const JointTransform& transform = skeleton->GetJointObjectSpaceTransform(jointIndex);
    Matrix4 jointTransform = transform.GetMatrix();
    return TransformUtils::ToMatrix(component->GetAttachmentTransform() * Transform(jointTransform));
}

void SlotSystem::SetScene(Scene* scene)
{
    SceneSystem::SetScene(scene);
    if (nullptr != externalEntityLoader)
    {
        externalEntityLoader->SetScene(scene);
    }
}

void SlotSystem::AttachEntityToSlotImpl(SlotComponent* component, Entity* entity, FastName itemName, SlotSystem::eSlotState state)
{
    component->loadedItemName = itemName;
    entity->SetName(component->GetSlotName());

    uint32 index = GetComponentIndex(component);
    DVASSERT(index >= loadedItemsCount);
    SlotNode& node = nodes[index];
    node.loadedEnity = entity;
    RaiseFlag(node, SlotNode::ATTACHMENT_TRANSFORM_CHANGED);
    SetState(node, state);
    std::swap(nodes[index], nodes[loadedItemsCount]);
    ++loadedItemsCount;
}

void SlotSystem::UnloadItem(SlotComponent* component)
{
    component->loadedItemName = FastName();
    uint32 index = GetComponentIndex(component);
    if (index < loadedItemsCount)
    {
        DVASSERT(loadedItemsCount > 0);
        SlotNode& node = nodes[index];
        Entity* loadedEntity = node.loadedEnity;
        Entity* parent = loadedEntity->GetParent();

        node.loadedEnity = nullptr;
        SetState(node, eSlotState::NOT_LOADED);

        std::swap(nodes[index], nodes[loadedItemsCount - 1]);
        --loadedItemsCount;

        parent->RemoveNode(loadedEntity);
    }
}

SlotSystem::eSlotState SlotSystem::GetSlotState(const SlotComponent* component) const
{
    return GetState(nodes[GetComponentIndex(component)]);
}

uint32 SlotSystem::GetComponentIndex(const SlotComponent* component) const
{
    auto iter = std::find_if(nodes.begin(), nodes.end(), [component](const SlotNode& node) {
        return node.component == component;
    });
    DVASSERT(iter != nodes.end(), "[SlotSystem::GetSlotState] Input component doesn't atteched to current scene");

    return static_cast<uint32>(std::distance(nodes.begin(), iter));
}

void SlotSystem::SetState(SlotNode& node, eSlotState state)
{
    node.flags = (node.flags & SlotNode::FLAGS_MASK) | static_cast<int32>(state);
}

SlotSystem::eSlotState SlotSystem::GetState(const SlotNode& node) const
{
    return static_cast<eSlotState>(node.flags & SlotNode::STATE_MASK);
}

bool SlotSystem::TestFlag(SlotNode& node, int32 flag) const
{
    DVASSERT((flag & SlotNode::FLAGS_MASK) == flag);
    return (node.flags & flag) == flag;
}

void SlotSystem::RaiseFlag(SlotNode& node, int32 flag)
{
    DVASSERT((flag & SlotNode::FLAGS_MASK) == flag);
    node.flags |= flag;
}

void SlotSystem::ResetFlag(SlotNode& node, int32 flag)
{
    DVASSERT((flag & SlotNode::FLAGS_MASK) == flag);
    node.flags = (node.flags & (~flag)) | (node.flags & SlotNode::STATE_MASK);
}

void SlotSystem::SetAttachmentTransform(SlotComponent* component, const Matrix4& transform)
{
    uint32 index = GetComponentIndex(component);
    RaiseFlag(nodes[index], SlotNode::ATTACHMENT_TRANSFORM_CHANGED);
    component->SetAttachmentTransform(transform);
}

} // namespace DAVA

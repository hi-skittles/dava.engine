#pragma once

#include "Entity/Component.h"

#include "Reflection/Reflection.h"

#include "FileSystem/FilePath.h"
#include "Math/Transform.h"
#include "Base/Array.h"
#include "Base/FastName.h"
#include "Base/IntrospectionBase.h"
#include "Scene3D/Components/SkeletonComponent.h"

namespace DAVA
{
class Entity;
/**
    \ingroup components
    \brief Component that used to declare a slot. External Entity can be loaded into slot.
     To do this, use SlotSystem::AttachItemToSlot or SlotSystem::AttachEntityToSlot method.
     Slot name (GetSlotName and SetSlotName) is used in SlotSystem::AttachItemToSlot to find all slots that item should be attached in.
     SlotComponent has attachment transform that declare local transform of loaded item in parent's space. To get attachment transform use
     GetAttachmentTransform. To set new attachment transform use SetAttachmentTransform.
     If entity that 'this' component attached in has SkeletonComponent, slot can be attached to one of Skeleton's joint
     In this case attachment transform will be declare local transform in joint's space. Use GetJointUID to check current attachment to joint and
     SetJointUID to attach slot to joint.
     Items that can be loaded into slot should be described in config file (GetConfigFilePath and SetConfigFilePath).
     System support 2 format: xml and yaml.
     XML format:
     \code{.xml}
         <items>
             <item Name="SomeItemName" Path="~res:/some_item.sc2" Type="Item">
             <item Name="OtherItemName" Path="~res:/OtherItems/other_item.sc2" Type="OtherItem">
         </items>
     \endcode
     
     YAML format:
     \code{.yaml}
         -
          Name: "SomeItemName"
          Path: "~res:/some_item.sc2"
          Type: "Item"
        -
          Name: "OtherItemName"
          Path: "~res:/OtherItems/other_item.sc2"
          Type: "OtherItem"
     \endcode

     Type filters is used in ResourceEditor to filtrate items that exist in config file, but can't be loaded into this slot
     SlotSystem ignore this filters in AttachItemToSlot method. It's decorative element.
*/
class SlotComponent : public Component
{
public:
    enum
    {
        MAX_FILTERS_COUNT = 8
    };

    Component* Clone(Entity* toEntity) override;
    void Serialize(KeyedArchive* archive, SerializationContext* serializationContext) override;
    void Deserialize(KeyedArchive* archive, SerializationContext* serializationContext) override;

    /** Getter for slot name */
    FastName GetSlotName() const;
    /**
        Setter for slot name
        \arg \c name new name of slot. Should be valid FastName
    */
    void SetSlotName(FastName name);

    /** Set attachment transform */
    void SetAttachmentTransform(const Transform& transform);

    /** Get attachment transform */
    const Transform& GetAttachmentTransform() const;

    /** Get UID of joint in SkeletonComponent of this Entity that slot attached to. If slot doesn't attached to joint return value is invalid */
    const FastName& GetJointUID() const;
    /** Set UID of joint in SkeletonComponent of this Entity that slot will be attached to. If \c jointUID is invalid that slot will be detached from join*/
    void SetJointUID(const FastName& jointUID);

    /** \return path to config file */
    const FilePath& GetConfigFilePath() const;
    /** \arg \c path path to config file that describe all items that can be loaded into slot */
    void SetConfigFilePath(const FilePath& path);

    uint32 GetTypeFiltersCount() const;
    FastName GetTypeFilter(uint32 index) const;
    void AddTypeFilter(FastName filter);
    void RemoveTypeFilter(uint32 index);
    void RemoveTypeFilter(FastName filter);

    /** \return Name of the item that currently loaded into 'this' slot*/
    FastName GetLoadedItemName() const;

    FastName GetTemplateName() const;

    static const FastName SlotNameFieldName;
    static const FastName ConfigPathFieldName;
    static const FastName AttchementToJointFieldName;
    static const FastName TemplateFieldName;

private:
    friend class SlotSystem;

    FastName slotName = FastName("");
    FastName templateName = FastName("");

    Transform attachmentTransform;
    FastName attachementToJoint;
    FilePath configFilePath;
    Array<FastName, MAX_FILTERS_COUNT> typeFilters;
    uint32 actualFiltersCount = 0;

    FastName loadedItemName;

    uint32 attachmentToJointIndex = SkeletonComponent::INVALID_JOINT_INDEX;

    DAVA_VIRTUAL_REFLECTION(SlotComponent, Component);
};

} // namespace DAVA

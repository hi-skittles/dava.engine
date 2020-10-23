#pragma once

#include <TArc/Controls/PropertyPanel/PropertyModelExtensions.h>

namespace PropertyPanel
{
enum eREPropertyType
{
    GroupQualityProperty = DAVA::PropertyNode::DomainSpecificProperty,
    AddComponentProperty,
    SlotName,
    SlotTypeFilters,
    SlotJointAttachment,
    SlotPreviewProperty,
    SlotTemplateName,
    SlotPasteProperty,
    CollisionPropertyType,
    CollisionPropertyTypeCrashed
};
} // namespace PropertyPanel

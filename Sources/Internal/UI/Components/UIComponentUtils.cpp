#include "UIComponentUtils.h"

#include "Reflection/Reflection.h"
#include "Reflection/ReflectedTypeDB.h"

namespace DAVA
{
namespace UIComponentUtilsDetails
{
template <typename MetaType>
const MetaType* GetTypeMeta(const Type* type)
{
    if (type == nullptr)
    {
        return nullptr;
    }

    const ReflectedType* refType = ReflectedTypeDB::GetByType(type);
    if (refType == nullptr)
    {
        return nullptr;
    }

    const ReflectedStructure* structure = refType->GetStructure();
    if (structure == nullptr || structure->meta == nullptr)
    {
        return nullptr;
    }

    return structure->meta->GetMeta<MetaType>();
}
}

bool UIComponentUtils::IsMultiple(const Type* componentType)
{
    return UIComponentUtilsDetails::GetTypeMeta<M::Multiple>(componentType) != nullptr;
}

bool UIComponentUtils::IsHidden(const Type* componentType)
{
    return UIComponentUtilsDetails::GetTypeMeta<M::HiddenField>(componentType) != nullptr;
}

String UIComponentUtils::GetDisplayName(const Type* componentType)
{
    const ReflectedType* refType = ReflectedTypeDB::GetByType(componentType);
    if (refType == nullptr)
    {
        return componentType->GetName();
    }

    const ReflectedStructure* structure = refType->GetStructure();
    if (structure == nullptr || structure->meta == nullptr)
    {
        return refType->GetPermanentName();
    }

    const M::DisplayName* displayName = structure->meta->GetMeta<M::DisplayName>();
    if (displayName)
    {
        return displayName->displayName;
    }
    return refType->GetPermanentName();
}

String UIComponentUtils::GetGroupName(const Type* componentType)
{
    const M::Group* group = UIComponentUtilsDetails::GetTypeMeta<M::Group>(componentType);
    return group ? group->groupName : "";
}
}

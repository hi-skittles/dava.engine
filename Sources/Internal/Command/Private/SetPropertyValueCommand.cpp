#include "Command/SetPropertyValueCommand.h"

#include "Base/Introspection.h"

namespace DAVA
{
SetPropertyValueCommand::SetPropertyValueCommand(const ObjectHandle& object_, const InspMember* property_, VariantType newValue_)
    : Command()
    , object(object_)
    , property(property_)
    , newValue(newValue_)
    , oldValue(property->Value(object.GetObjectPointer()))

{
    DVASSERT(object.IsValid() == true);
    DVASSERT(object.GetIntrospection() != nullptr);
    DVASSERT(object.GetIntrospection()->Member(property->Name()) != nullptr);

    const DAVA::MetaInfo* propertyType = property->Type();
    if (newValue.Meta() != propertyType)
    {
        newValue = VariantType::Convert(newValue, propertyType);
        DVASSERT(newValue.Meta() == propertyType);
    }
}

void SetPropertyValueCommand::Redo()
{
    property->SetValue(object.GetObjectPointer(), newValue);
}

void SetPropertyValueCommand::Undo()
{
    property->SetValue(object.GetObjectPointer(), oldValue);
}
}

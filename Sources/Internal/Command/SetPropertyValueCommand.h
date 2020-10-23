#pragma once

#include "Command/Command.h"
#include "Command/ObjectHandle.h"
#include "FileSystem/VariantType.h"

namespace DAVA
{
class InspMember;

class SetPropertyValueCommand : public Command
{
public:
    SetPropertyValueCommand(const ObjectHandle& object, const InspMember* property, VariantType newValue);

    void Redo() override;
    void Undo() override;

private:
    ObjectHandle object;
    const InspMember* property;
    VariantType newValue;
    VariantType oldValue;
};
}

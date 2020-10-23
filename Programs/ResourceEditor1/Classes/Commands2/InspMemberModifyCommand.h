#ifndef __INSP_MEMEBER_MODIFY_COMMAND_H__
#define __INSP_MEMEBER_MODIFY_COMMAND_H__

#include "Commands2/Base/RECommand.h"
#include "FileSystem/VariantType.h"

namespace DAVA
{
class InspMember;
}

class InspMemberModifyCommand : public RECommand
{
public:
    InspMemberModifyCommand(const DAVA::InspMember* member, void* object, const DAVA::VariantType& value);
    ~InspMemberModifyCommand();

    void Undo() override;
    void Redo() override;

    const DAVA::InspMember* member;
    void* object;

    DAVA::VariantType oldValue;
    DAVA::VariantType newValue;
};

#endif // __INSP_MEMEBER_MODIFY_COMMAND_H__

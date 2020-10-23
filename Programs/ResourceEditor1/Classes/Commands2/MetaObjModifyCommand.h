#ifndef __META_OBJ_MODIFY_COMMAND_H__
#define __META_OBJ_MODIFY_COMMAND_H__

#include "Commands2/Base/RECommand.h"
#include "FileSystem/VariantType.h"

namespace DAVA
{
struct MetaInfo;
}

class MetaObjModifyCommand : public RECommand
{
public:
    MetaObjModifyCommand(const DAVA::MetaInfo* info, void* object, const DAVA::VariantType& value);
    ~MetaObjModifyCommand();

    void Undo() override;
    void Redo() override;

    const DAVA::MetaInfo* info;
    void* object;

    DAVA::VariantType oldValue;
    DAVA::VariantType newValue;
};

#endif // __META_OBJ_MODIFY_COMMAND_H__

#include "Commands2/MetaObjModifyCommand.h"
#include "Commands2/RECommandIDs.h"

MetaObjModifyCommand::MetaObjModifyCommand(const DAVA::MetaInfo* _info, void* _object, const DAVA::VariantType& _newValue)
    : RECommand(CMDID_META_OBJ_MODIFY, "Modify value")
    , info(_info)
    , object(_object)
    , newValue(_newValue)
{
    if (NULL != info && NULL != object)
    {
        oldValue = DAVA::VariantType::LoadData(object, info);
    }
}

MetaObjModifyCommand::~MetaObjModifyCommand()
{
}

void MetaObjModifyCommand::Undo()
{
    if (NULL != info && NULL != object)
    {
        DAVA::VariantType::SaveData(object, info, oldValue);
    }
}

void MetaObjModifyCommand::Redo()
{
    if (NULL != info && NULL != object)
    {
        DAVA::VariantType::SaveData(object, info, newValue);
    }
}

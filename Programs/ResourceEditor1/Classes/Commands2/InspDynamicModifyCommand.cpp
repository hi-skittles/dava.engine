#include "Commands2/InspDynamicModifyCommand.h"
#include "Commands2/RECommandIDs.h"

InspDynamicModifyCommand::InspDynamicModifyCommand(DAVA::InspInfoDynamic* _dynamicInfo, const DAVA::InspInfoDynamic::DynamicData& _ddata, DAVA::FastName _key, const DAVA::VariantType& _newValue)
    : RECommand(CMDID_INSP_DYNAMIC_MODIFY, "Modify dynamic value")
    , dynamicInfo(_dynamicInfo)
    , key(_key)
    , ddata(_ddata)
    , newValue(_newValue)
{
    if (nullptr != dynamicInfo)
    {
        // if value can't be edited, it means that it was inherited
        // so don't retrieve oldValue, but leave it as uninitialized variant
        if (dynamicInfo->MemberFlags(ddata, key) & DAVA::I_EDIT)
        {
            oldValue = dynamicInfo->MemberValueGet(ddata, key);
        }
    }
}

InspDynamicModifyCommand::~InspDynamicModifyCommand()
{
}

void InspDynamicModifyCommand::Undo()
{
    if (nullptr != dynamicInfo)
    {
        dynamicInfo->MemberValueSet(ddata, key, oldValue);
    }
}

void InspDynamicModifyCommand::Redo()
{
    if (nullptr != dynamicInfo)
    {
        dynamicInfo->MemberValueSet(ddata, key, newValue);
    }
}

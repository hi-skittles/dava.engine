#ifndef __INSP_DYNAMIC_MODIFY_COMMAND_H__
#define __INSP_DYNAMIC_MODIFY_COMMAND_H__

#include "Commands2/Base/RECommand.h"

#include "FileSystem/VariantType.h"
#include "Base/FastName.h"
#include "Base/Introspection.h"

class InspDynamicModifyCommand : public RECommand
{
public:
    InspDynamicModifyCommand(DAVA::InspInfoDynamic* dynamicInfo, const DAVA::InspInfoDynamic::DynamicData& ddata, DAVA::FastName key, const DAVA::VariantType& value);
    ~InspDynamicModifyCommand();

    void Undo() override;
    void Redo() override;

    DAVA::InspInfoDynamic* dynamicInfo;
    DAVA::FastName key;

    DAVA::InspInfoDynamic::DynamicData ddata;

    DAVA::VariantType oldValue;
    DAVA::VariantType newValue;
};

#endif // __INSP_DYNAMIC_MODIFY_COMMAND_H__

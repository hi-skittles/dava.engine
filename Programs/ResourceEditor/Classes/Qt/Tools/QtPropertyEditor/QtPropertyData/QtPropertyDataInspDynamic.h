#ifndef __QT_PROPERTY_DATA_INSP_DYNAMIC_H__
#define __QT_PROPERTY_DATA_INSP_DYNAMIC_H__

#include "Classes/Qt/Tools/QtPropertyEditor/QtPropertyData.h"
#include "Classes/Qt/Tools/QtPropertyEditor/QtPropertyData/QtPropertyDataDavaVariant.h"

#include <REPlatform/Commands/InspDynamicModifyCommand.h>

#include <Base/Introspection.h>
#include <Base/FastName.h>

class QtPropertyDataInspDynamic : public QtPropertyDataDavaVariant
{
public:
    QtPropertyDataInspDynamic(const DAVA::FastName& name, DAVA::InspInfoDynamic* _dynamicInfo, DAVA::InspInfoDynamic::DynamicData _ddata);
    virtual ~QtPropertyDataInspDynamic();

    int InspFlags() const;

    const DAVA::MetaInfo* MetaInfo() const override;
    std::unique_ptr<DAVA::Command> CreateLastCommand() const override;

    DAVA::InspInfoDynamic* GetDynamicInfo() const
    {
        return dynamicInfo;
    }

    DAVA::VariantType GetVariant() const
    {
        return dynamicInfo->MemberValueGet(ddata, name);
    }

    DAVA::VariantType GetAliasVariant() const
    {
        return dynamicInfo->MemberAliasGet(ddata, name);
    }

    DAVA::FastName name;
    DAVA::InspInfoDynamic* dynamicInfo;
    DAVA::InspInfoDynamic::DynamicData ddata;

protected:
    int inspFlags;
    DAVA::InspDynamicModifyCommand* lastCommand;

    QVariant GetValueAlias() const override;
    void SetValueInternal(const QVariant& value) override;
    void SetTempValueInternal(const QVariant& value) override;
    bool UpdateValueInternal() override;
    bool EditorDoneInternal(QWidget* editor) override;
};

#endif // __QT_PROPERTY_DATA_INSP_DYNAMIC_H__

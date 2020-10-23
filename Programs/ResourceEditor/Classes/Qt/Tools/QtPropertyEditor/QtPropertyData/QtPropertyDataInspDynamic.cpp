#include "QtPropertyDataInspDynamic.h"

#include <REPlatform/Commands/InspDynamicModifyCommand.h>

#include <QDebug>

QtPropertyDataInspDynamic::QtPropertyDataInspDynamic(const DAVA::FastName& name_, DAVA::InspInfoDynamic* _dynamicInfo, DAVA::InspInfoDynamic::DynamicData _ddata)
    : QtPropertyDataDavaVariant(name_, DAVA::VariantType())
    , name(name_)
    , dynamicInfo(_dynamicInfo)
    , ddata(_ddata)
    , inspFlags(0)
    , lastCommand(NULL)
{
    if (NULL != dynamicInfo)
    {
        SetVariantValue(dynamicInfo->MemberValueGet(ddata, name));
        inspFlags = dynamicInfo->MemberFlags(ddata, name);
    }
}

QtPropertyDataInspDynamic::~QtPropertyDataInspDynamic()
{
    DAVA::SafeDelete(lastCommand);
}

const DAVA::MetaInfo* QtPropertyDataInspDynamic::MetaInfo() const
{
    if (NULL != dynamicInfo && NULL != dynamicInfo->GetMember())
    {
        return dynamicInfo->GetMember()->Type();
    }

    return NULL;
}

int QtPropertyDataInspDynamic::InspFlags() const
{
    return inspFlags;
}

QVariant QtPropertyDataInspDynamic::GetValueAlias() const
{
    QVariant ret;

    if (NULL != dynamicInfo)
    {
        ret = FromDavaVariant(dynamicInfo->MemberAliasGet(ddata, name));
    }

    return ret;
}

void QtPropertyDataInspDynamic::SetValueInternal(const QVariant& value)
{
    QtPropertyDataDavaVariant::SetValueInternal(value);
    DAVA::VariantType newValue;

    if (!value.isNull())
    {
        newValue = QtPropertyDataDavaVariant::GetVariantValue();
    }

    // also save value to meta-object
    if (NULL != dynamicInfo)
    {
        DAVA::SafeDelete(lastCommand);
        lastCommand = new DAVA::InspDynamicModifyCommand(dynamicInfo, ddata, name, newValue);

        dynamicInfo->MemberValueSet(ddata, name, newValue);
    }
}

void QtPropertyDataInspDynamic::SetTempValueInternal(const QVariant& value)
{
    QtPropertyDataDavaVariant::SetValueInternal(value);
    DAVA::VariantType newValue;

    if (!value.isNull())
    {
        newValue = QtPropertyDataDavaVariant::GetVariantValue();
    }

    // save value to meta-object
    if (NULL != dynamicInfo)
    {
        dynamicInfo->MemberValueSet(ddata, name, newValue);
    }
}

bool QtPropertyDataInspDynamic::UpdateValueInternal()
{
    bool ret = false;

    // get current value from introspection member
    // we should do this because member may change at any time
    if (NULL != dynamicInfo)
    {
        DAVA::VariantType v = dynamicInfo->MemberValueGet(ddata, name);

        // if current variant value not equal to the real member value
        // we should update current variant value
        if (v.GetType() != DAVA::VariantType::TYPE_NONE && v != GetVariantValue())
        {
            QtPropertyDataDavaVariant::SetVariantValue(v);
            ret = true;
        }
    }

    return ret;
}

bool QtPropertyDataInspDynamic::EditorDoneInternal(QWidget* editor)
{
    bool ret = QtPropertyDataDavaVariant::EditorDoneInternal(editor);

    // if there was some changes in current value, done by editor
    // we should save them into meta-object
    if (ret && NULL != dynamicInfo)
    {
        dynamicInfo->MemberValueSet(ddata, name, QtPropertyDataDavaVariant::GetVariantValue());
    }

    return ret;
}

std::unique_ptr<DAVA::Command> QtPropertyDataInspDynamic::CreateLastCommand() const
{
    if (nullptr != lastCommand)
    {
        return std::unique_ptr<DAVA::Command>(new DAVA::InspDynamicModifyCommand(*lastCommand));
    }

    return std::unique_ptr<DAVA::Command>();
}

#include "Classes/Qt/Tools/QtPropertyEditor/QtPropertyData/QtPropertyDataMetaObject.h"

#include <REPlatform/Commands/MetaObjModifyCommand.h>

QtPropertyDataMetaObject::QtPropertyDataMetaObject(const DAVA::FastName& name, void* _object, const DAVA::MetaInfo* _meta)
    : QtPropertyDataDavaVariant(name, DAVA::VariantType::LoadData(_object, _meta))
    , object(_object)
    , meta(_meta)
    , lastCommand(NULL)
{
}

QtPropertyDataMetaObject::~QtPropertyDataMetaObject()
{
    DAVA::SafeDelete(lastCommand);
}

const DAVA::MetaInfo* QtPropertyDataMetaObject::MetaInfo() const
{
    return meta;
}

void QtPropertyDataMetaObject::SetValueInternal(const QVariant& value)
{
    QtPropertyDataDavaVariant::SetValueInternal(value);
    DAVA::VariantType newValue = QtPropertyDataDavaVariant::GetVariantValue();

    DAVA::SafeDelete(lastCommand);
    lastCommand = new DAVA::MetaObjModifyCommand(meta, object, newValue);

    // also save value to meta-object
    DAVA::VariantType::SaveData(object, meta, newValue);
}

bool QtPropertyDataMetaObject::UpdateValueInternal()
{
    bool ret = false;

    // load current value from meta-object
    // we should do this because meta-object may change at any time
    DAVA::VariantType v = DAVA::VariantType::LoadData(object, meta);

    // if current variant value not equel to the real meta-object value
    // we should update current variant value
    if (v != GetVariantValue())
    {
        QtPropertyDataDavaVariant::SetVariantValue(v);
        ret = true;
    }

    return ret;
}

bool QtPropertyDataMetaObject::EditorDoneInternal(QWidget* editor)
{
    bool ret = QtPropertyDataDavaVariant::EditorDoneInternal(editor);

    // if there was some changes in current value, done by editor
    // we should save them into meta-object
    if (ret)
    {
        DAVA::VariantType::SaveData(object, meta, QtPropertyDataDavaVariant::GetVariantValue());
    }

    return ret;
}

std::unique_ptr<DAVA::Command> QtPropertyDataMetaObject::CreateLastCommand() const
{
    if (nullptr != lastCommand)
    {
        return std::unique_ptr<DAVA::Command>(new DAVA::MetaObjModifyCommand(*lastCommand));
    }

    return std::unique_ptr<DAVA::Command>();
}

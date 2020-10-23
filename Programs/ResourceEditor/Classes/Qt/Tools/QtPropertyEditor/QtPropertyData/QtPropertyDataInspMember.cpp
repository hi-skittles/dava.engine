#include "QtPropertyDataInspMember.h"
#include <REPlatform/Commands/InspMemberModifyCommand.h>

QtPropertyDataInspMember::QtPropertyDataInspMember(const DAVA::FastName& name, void* _object, const DAVA::InspMember* _member)
    : QtPropertyDataDavaVariant(name, DAVA::VariantType())
    , object(_object)
    , member(_member)
    , lastCommand(NULL)
{
    if (NULL != member)
    {
        SetVariantValue(member->Value(object));
    }
}

QtPropertyDataInspMember::~QtPropertyDataInspMember()
{
    DAVA::SafeDelete(lastCommand);
}

const DAVA::MetaInfo* QtPropertyDataInspMember::MetaInfo() const
{
    if (NULL != member)
    {
        return member->Type();
    }

    return NULL;
}

void QtPropertyDataInspMember::SetValueInternal(const QVariant& value)
{
    QtPropertyDataDavaVariant::SetValueInternal(value);
    DAVA::VariantType newValue = QtPropertyDataDavaVariant::GetVariantValue();

    // also save value to meta-object
    if (NULL != member)
    {
        DAVA::SafeDelete(lastCommand);
        lastCommand = new DAVA::InspMemberModifyCommand(member, object, newValue);

        member->SetValue(object, newValue);
    }
}

void QtPropertyDataInspMember::SetTempValueInternal(const QVariant& value)
{
    QtPropertyDataDavaVariant::SetValueInternal(value);
    DAVA::VariantType newValue = GetVariantValue();

    // save value to meta-object
    if (NULL != member)
    {
        member->SetValue(object, newValue);
    }
}

bool QtPropertyDataInspMember::UpdateValueInternal()
{
    bool ret = false;

    // get current value from introspection member
    // we should do this because member may change at any time
    if (NULL != member)
    {
        DAVA::VariantType v = member->Value(object);

        // if current variant value not equal to the real member value
        // we should update current variant value
        if (v != GetVariantValue())
        {
            QtPropertyDataDavaVariant::SetVariantValue(v);
            ret = true;
        }
    }

    return ret;
}

bool QtPropertyDataInspMember::EditorDoneInternal(QWidget* editor)
{
    bool ret = QtPropertyDataDavaVariant::EditorDoneInternal(editor);

    // if there was some changes in current value, done by editor
    // we should save them into meta-object
    if (ret && NULL != member)
    {
        member->SetValue(object, QtPropertyDataDavaVariant::GetVariantValue());
    }

    return ret;
}

std::unique_ptr<DAVA::Command> QtPropertyDataInspMember::CreateLastCommand() const
{
    if (nullptr != lastCommand)
    {
        return std::unique_ptr<DAVA::Command>(new DAVA::InspMemberModifyCommand(*lastCommand));
    }

    return std::unique_ptr<DAVA::Command>();
}

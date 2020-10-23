#include "QtPropertyDataKeyedArchiveMember.h"

#include <REPlatform/DataNodes/ProjectManagerData.h>
#include <REPlatform/Deprecated/EditorConfig.h>

#include <REPlatform/Commands/KeyedArchiveCommand.h>
#include <TArc/Utils/Utils.h>
#include <TArc/Core/Deprecated.h>

#include <FileSystem/KeyedArchive.h>

QtPropertyKeyedArchiveMember::QtPropertyKeyedArchiveMember(const DAVA::FastName& name, DAVA::KeyedArchive* archive_, const DAVA::String& key_)
    : QtPropertyDataDavaVariant(name, DAVA::VariantType())
    , archive(archive_)
    , key(key_.c_str())
    , lastCommand(NULL)
{
    CheckAndFillPresetValues();

    if (NULL != archive)
    {
        DAVA::VariantType* val = archive->GetVariant(key);
        if (NULL != val)
        {
            SetVariantValue(*val);
        }
    }
}

QtPropertyKeyedArchiveMember::~QtPropertyKeyedArchiveMember()
{
    DAVA::SafeDelete(lastCommand);
}

void QtPropertyKeyedArchiveMember::CheckAndFillPresetValues()
{
    const int valueType = archive->GetVariant(key)->GetType();

    DAVA::ProjectManagerData* data = DAVA::Deprecated::GetDataNode<DAVA::ProjectManagerData>();
    DVASSERT(data);

    const DAVA::EditorConfig* editorConfig = data->GetEditorConfig();
    const int presetValueType = editorConfig->GetPropertyValueType(key);
    if (presetValueType != DAVA::VariantType::TYPE_NONE)
    {
        if (valueType == presetValueType)
        {
            const DAVA::Vector<DAVA::String>& allowedValues = editorConfig->GetComboPropertyValues(key);
            if (allowedValues.size() > 0)
            {
                for (size_t i = 0; i < allowedValues.size(); ++i)
                {
                    AddAllowedValue(DAVA::VariantType((int)i), allowedValues[i].c_str());
                }
            }
            else
            {
                const DAVA::Vector<DAVA::Color>& allowedColors = editorConfig->GetColorPropertyValues(key);
                for (size_t i = 0; i < allowedColors.size(); ++i)
                {
                    AddAllowedValue(DAVA::VariantType((int)i), DAVA::ColorToQColor(allowedColors[i]));
                }
            }
        }
    }
}

void QtPropertyKeyedArchiveMember::SetValueInternal(const QVariant& value)
{
    QtPropertyDataDavaVariant::SetValueInternal(value);
    DAVA::VariantType newValue = QtPropertyDataDavaVariant::GetVariantValue();

    // also save value to meta-object
    if (NULL != archive && archive->IsKeyExists(key))
    {
        DAVA::SafeDelete(lastCommand);
        lastCommand = new DAVA::KeyeadArchiveSetValueCommand(archive, key, newValue);

        archive->SetVariant(key, newValue);
    }
}

bool QtPropertyKeyedArchiveMember::UpdateValueInternal()
{
    bool ret = false;

    // get current value from introspection member
    // we should do this because member may change at any time
    if (NULL != archive)
    {
        DAVA::VariantType* val = archive->GetVariant(key);
        if (NULL != val)
        {
            DAVA::VariantType v = *val;

            // if current variant value not equal to the real member value
            // we should update current variant value
            if (v != GetVariantValue())
            {
                QtPropertyDataDavaVariant::SetVariantValue(v);
                ret = true;
            }
        }
    }

    return ret;
}

bool QtPropertyKeyedArchiveMember::EditorDoneInternal(QWidget* editor)
{
    bool ret = QtPropertyDataDavaVariant::EditorDoneInternal(editor);

    // if there was some changes in current value, done by editor
    // we should save them into meta-object
    if (ret && NULL != archive && archive->IsKeyExists(key))
    {
        archive->SetVariant(key, QtPropertyDataDavaVariant::GetVariantValue());
    }

    return ret;
}

std::unique_ptr<DAVA::Command> QtPropertyKeyedArchiveMember::CreateLastCommand() const
{
    if (nullptr != lastCommand)
    {
        return std::unique_ptr<DAVA::Command>(new DAVA::KeyeadArchiveSetValueCommand(*lastCommand));
    }

    return std::unique_ptr<DAVA::Command>();
}

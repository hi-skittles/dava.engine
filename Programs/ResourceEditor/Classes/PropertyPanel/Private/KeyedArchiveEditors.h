#pragma once

#include <TArc/Controls/PropertyPanel/BaseComponentValue.h>
#include <TArc/Utils/QtConnections.h>

#include <FileSystem/VariantType.h>
#include <Reflection/Reflection.h>
#include <Base/Any.h>
#include <Base/BaseTypes.h>

namespace PropertyPanel
{
class AddKeyedArchiveItemWidget;
class KeyedArchiveEditor : public DAVA::BaseComponentValue
{
public:
    KeyedArchiveEditor(const DAVA::Vector<DAVA::String>& presetNames, const DAVA::Vector<DAVA::VariantType>& defaultValues);
    ~KeyedArchiveEditor() override;

protected:
    DAVA::Any GetMultipleValue() const override;
    bool IsValidValueToSet(const DAVA::Any& newValue, const DAVA::Any& currentValue) const override;

    DAVA::ControlProxy* CreateEditorWidget(QWidget* parent, const DAVA::Reflection& model, DAVA::DataWrappersProcessor* wrappersProcessor) override;

    void OnCreatePropertyClicked();
    void OnCreatePresetPropertyClicked();
    void AddProperty(const DAVA::String& key, const DAVA::VariantType& value);

    bool IsPresetChoosed() const;
    DAVA::int32 GetChoosedPreset() const;
    void SetChoosedPreset(DAVA::int32 choosedPreset);

private:
    DAVA::QtConnections connections;
    static int lastAddedType;
    QPointer<AddKeyedArchiveItemWidget> widget;

    DAVA::Vector<DAVA::String> presetNames;
    DAVA::Vector<DAVA::VariantType> defaultValues;
    DAVA::int32 choosedPreset;

    DAVA_VIRTUAL_REFLECTION(KeyedArchiveEditor, DAVA::BaseComponentValue);
};

class KeyedArchiveComboPresetEditor : public DAVA::BaseComponentValue
{
public:
    KeyedArchiveComboPresetEditor(const DAVA::Vector<DAVA::Any>& values);

protected:
    DAVA::Any GetMultipleValue() const override;
    bool IsValidValueToSet(const DAVA::Any& newValue, const DAVA::Any& currentValue) const override;
    DAVA::ControlProxy* CreateEditorWidget(QWidget* parent, const DAVA::Reflection& model, DAVA::DataWrappersProcessor* wrappersProcessor) override;

private:
    DAVA::Any GetValueAny() const;
    void SetValueAny(const DAVA::Any& newValue);

    DAVA::Vector<DAVA::Any> allowedValues;

    DAVA_VIRTUAL_REFLECTION(KeyedArchiveComboPresetEditor, DAVA::BaseComponentValue);
};

} // namespace PropertyPanel

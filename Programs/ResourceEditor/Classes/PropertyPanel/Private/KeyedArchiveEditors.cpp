#include "Classes/PropertyPanel/Private/KeyedArchiveEditors.h"

#include <REPlatform/Commands/KeyedArchiveCommand.h>
#include <REPlatform/DataNodes/ProjectManagerData.h>
#include <REPlatform/Deprecated/EditorConfig.h>

#include <TArc/Controls/CheckBox.h>
#include <TArc/Controls/ComboBox.h>
#include <TArc/Controls/CommonStrings.h>
#include <TArc/Controls/LineEdit.h>
#include <TArc/Controls/QtBoxLayouts.h>
#include <TArc/Controls/ReflectedButton.h>
#include <TArc/Controls/Widget.h>
#include <TArc/Core/ContextAccessor.h>
#include <TArc/Qt/QtSize.h>
#include <TArc/Utils/Utils.h>

#include <Base/BaseTypes.h>
#include <Base/RefPtr.h>
#include <FileSystem/KeyedArchive.h>
#include <Functional/Signal.h>
#include <Reflection/ReflectionRegistrator.h>

#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QtEvents>
#include <QToolButton>
#include <QWidget>

namespace PropertyPanel
{
class AddKeyedArchiveItemWidget : public QWidget
{
public:
    AddKeyedArchiveItemWidget(DAVA::ContextAccessor* accessor_, DAVA::UI* ui, const DAVA::WindowKey& wndKey,
                              DAVA::Vector<DAVA::RefPtr<DAVA::KeyedArchive>>&& archives_, DAVA::int32 lastAddedType)
        : type(lastAddedType)
        , accessor(accessor_)
        , archives(std::move(archives_))
    {
        using namespace DAVA;
        using namespace DAVA;

        PrepareData();

        if (type <= VariantType::TYPE_NONE || type >= VariantType::TYPES_COUNT)
        {
            type = VariantType::TYPE_STRING;
        }

        QGridLayout* layout = new QGridLayout(this);
        layout->setMargin(5);
        layout->setSpacing(3);

        layout->addWidget(new QLabel("Key:", this), 0, 0, 1, 1);
        layout->addWidget(new QLabel("Value type:", this), 1, 0, 1, 1);
        layout->addWidget(new QLabel("Preset:", this), 2, 0, 1, 1);

        Reflection r = Reflection::Create(ReflectedObject(this));

        {
            LineEdit::Params params(accessor, ui, wndKey);
            params.fields[LineEdit::Fields::IsEnabled] = "isKeyEnabled";
            params.fields[LineEdit::Fields::Text] = "key";
            LineEdit* keyEdit = new LineEdit(params, accessor, r, this);
            keyEdit->SetObjectName(QString("keyEdit"));
            layout->addWidget(keyEdit->ToWidgetCast(), 0, 1, 1, 2);
            lineEdit = keyEdit->ToWidgetCast();
            setFocusProxy(lineEdit);
        }

        {
            ComboBox::Params params(accessor, ui, wndKey);
            params.fields[ComboBox::Fields::Enumerator] = "types";
            params.fields[ComboBox::Fields::Value] = "type";
            params.fields[ComboBox::Fields::IsReadOnly] = "isTypeDisabled";
            ComboBox* typesCombo = new ComboBox(params, accessor, r, this);
            layout->addWidget(typesCombo->ToWidgetCast(), 1, 1, 1, 2);
        }

        {
            ComboBox::Params params(accessor, ui, wndKey);
            params.fields[ComboBox::Fields::Enumerator] = "presets";
            params.fields[ComboBox::Fields::Value] = "presetIndex";
            ComboBox* typesCombo = new ComboBox(params, accessor, r, this);
            layout->addWidget(typesCombo->ToWidgetCast(), 2, 1, 1, 2);
        }

        QPushButton* button = new QPushButton(QStringLiteral("Add property"), this);
        button->setObjectName("addPropertyButton");
        connections.AddConnection(button, &QPushButton::clicked, MakeFunction(this, &AddKeyedArchiveItemWidget::OnButtonClicked));
        layout->addWidget(button, 3, 1, 1, 2);

        setAttribute(Qt::WA_DeleteOnClose);
        setWindowFlags(Qt::FramelessWindowHint | Qt::Popup);
        setWindowOpacity(0.95);
    }

    void Show()
    {
        show();
        lineEdit->setFocus();
    }

    DAVA::Signal<const DAVA::String&, const DAVA::VariantType&> commitAddPropperty;

private:
    void keyPressEvent(QKeyEvent* e)
    {
        if (!e->modifiers() || (e->modifiers() & Qt::KeypadModifier && e->key() == Qt::Key_Enter))
        {
            switch (e->key())
            {
            case Qt::Key_Enter:
            case Qt::Key_Return:
                OnButtonClicked();
                break;
            case Qt::Key_Escape:
                this->deleteLater();
                break;
            default:
                e->ignore();
                return;
            }
        }
        else
        {
            e->ignore();
        }
    }

    void OnButtonClicked()
    {
        if (key.empty())
        {
            return;
        }

        if (presetIndex != 0)
        {
            DAVA::ProjectManagerData* data = accessor->GetGlobalContext()->GetData<DAVA::ProjectManagerData>();
            DVASSERT(data);

            const DAVA::EditorConfig* editorConfig = data->GetEditorConfig();
            const DAVA::VariantType* v = editorConfig->GetPropertyDefaultValue(key);
            commitAddPropperty.Emit(key, *v);
        }
        else
        {
            commitAddPropperty.Emit(key, DAVA::VariantType::FromType(DAVA::VariantType::variantNamesMap[type].variantMeta));
        }

        close();
    }

    void PrepareData()
    {
        DAVA::ProjectManagerData* data = accessor->GetGlobalContext()->GetData<DAVA::ProjectManagerData>();
        DVASSERT(data);

        const DAVA::EditorConfig* editorConfig = data->GetEditorConfig();
        const DAVA::Vector<DAVA::String>& presetValues = editorConfig->GetProjectPropertyNames();
        presets.push_back("None");
        presets.insert(presets.end(), presetValues.begin(), presetValues.end());

        for (DAVA::int32 type = DAVA::VariantType::TYPE_NONE + 1; type < DAVA::VariantType::TYPES_COUNT; type++)
        {
            bool ignoreType = false;
            switch (type)
            {
            case DAVA::VariantType::TYPE_BYTE_ARRAY:
            case DAVA::VariantType::TYPE_KEYED_ARCHIVE:
            case DAVA::VariantType::TYPE_MATRIX2:
            case DAVA::VariantType::TYPE_MATRIX3:
            case DAVA::VariantType::TYPE_MATRIX4:
            case DAVA::VariantType::TYPE_INT8:
            case DAVA::VariantType::TYPE_UINT8:
            case DAVA::VariantType::TYPE_INT16:
            case DAVA::VariantType::TYPE_UINT16:
            case DAVA::VariantType::TYPE_INT64:
            case DAVA::VariantType::TYPE_UINT64:
                ignoreType = true;
            default:
                break;
            }

            if (ignoreType == true)
            {
                continue;
            }

            types[type] = DAVA::VariantType::variantNamesMap[type].variantName;
        }
    }

    const DAVA::String& GetKey() const
    {
        return key;
    }

    void SetKey(const DAVA::String& key_)
    {
        key = key_;
    }

    bool IsKeyEnabled() const
    {
        return presetIndex == 0;
    }

    bool IsTypeDisabled() const
    {
        return presetIndex != 0;
    }

    DAVA::int32 GetPreset() const
    {
        return presetIndex;
    }

    void SetPreset(DAVA::int32 preset)
    {
        presetIndex = preset;
        if (presetIndex == 0)
        {
            key.clear();
        }
        else
        {
            SetKey(presets[presetIndex]);

            DAVA::ProjectManagerData* data = accessor->GetGlobalContext()->GetData<DAVA::ProjectManagerData>();
            DVASSERT(data);

            const DAVA::EditorConfig* editorConfig = data->GetEditorConfig();
            type = editorConfig->GetPropertyValueType(key);
        }
    }

    DAVA::String key = "";
    DAVA::int32 type = DAVA::VariantType::TYPE_STRING;
    DAVA::int32 presetIndex = 0;

    DAVA::Map<DAVA::int32, DAVA::String> types;
    DAVA::Vector<DAVA::String> presets;

    DAVA::ContextAccessor* accessor;
    DAVA::Vector<DAVA::RefPtr<DAVA::KeyedArchive>> archives;
    QWidget* lineEdit = nullptr;

    DAVA::QtConnections connections;

    DAVA_REFLECTION(AddKeyedArchiveItemWidget);
};

DAVA_REFLECTION_IMPL(AddKeyedArchiveItemWidget)
{
    DAVA::ReflectionRegistrator<AddKeyedArchiveItemWidget>::Begin()
    .Field("key", &AddKeyedArchiveItemWidget::GetKey, &AddKeyedArchiveItemWidget::SetKey)
    .Field("isKeyEnabled", &AddKeyedArchiveItemWidget::IsKeyEnabled, nullptr)
    .Field("type", &AddKeyedArchiveItemWidget::type)
    .Field("isTypeDisabled", &AddKeyedArchiveItemWidget::IsTypeDisabled, nullptr)
    .Field("types", &AddKeyedArchiveItemWidget::types)
    .Field("presetIndex", &AddKeyedArchiveItemWidget::GetPreset, &AddKeyedArchiveItemWidget::SetPreset)
    .Field("presets", &AddKeyedArchiveItemWidget::presets)
    .End();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                      KeyedArchiveEditor                                               //
///////////////////////////////////////////////////////////////////////////////////////////////////////////

KeyedArchiveEditor::KeyedArchiveEditor(const DAVA::Vector<DAVA::String>& presetNames_, const DAVA::Vector<DAVA::VariantType>& defaultValues_)
    : presetNames(presetNames_)
    , defaultValues(defaultValues_)
{
    DVASSERT(presetNames.size() == defaultValues.size());
    choosedPreset = -1;
}

KeyedArchiveEditor::~KeyedArchiveEditor()
{
    if (widget.isNull() == false)
    {
        widget->commitAddPropperty.DisconnectAll();
        widget->deleteLater();
    }
}

DAVA::Any KeyedArchiveEditor::GetMultipleValue() const
{
    return DAVA::Any();
}

bool KeyedArchiveEditor::IsValidValueToSet(const DAVA::Any& newValue, const DAVA::Any& currentValue) const
{
    return false;
}

DAVA::ControlProxy* KeyedArchiveEditor::CreateEditorWidget(QWidget* parent, const DAVA::Reflection& model, DAVA::DataWrappersProcessor* wrappersProcessor)
{
    using namespace DAVA;
    using namespace DAVA;

    Widget* w = new Widget(parent);
    QtHBoxLayout* layout = new QtHBoxLayout(w->ToWidgetCast());
    layout->setMargin(0);
    layout->setSpacing(1);
    {
        ReflectedButton::Params params(GetAccessor(), GetUI(), GetWindowKey());
        params.fields[ReflectedButton::Fields::Clicked] = "createPropertyValue";
        params.fields[ReflectedButton::Fields::Icon] = "createPropertyButtonIcon";
        params.fields[ReflectedButton::Fields::IconSize] = "createPropertyButtonIconSize";
        params.fields[ReflectedButton::Fields::AutoRaise] = "buttonAutoRise";
        ReflectedButton* button = new ReflectedButton(params, wrappersProcessor, model, w->ToWidgetCast());
        w->AddControl(button);
    }
    {
        ReflectedButton::Params params(GetAccessor(), GetUI(), GetWindowKey());
        params.fields[ReflectedButton::Fields::Clicked] = "createPresetValue";
        params.fields[ReflectedButton::Fields::Icon] = "createPresetButtonIcon";
        params.fields[ReflectedButton::Fields::IconSize] = "createPropertyButtonIconSize";
        params.fields[ReflectedButton::Fields::AutoRaise] = "buttonAutoRise";
        params.fields[ReflectedButton::Fields::Enabled] = "isPresetChoosed";

        ReflectedButton* button = new ReflectedButton(params, wrappersProcessor, model, w->ToWidgetCast());
        w->AddControl(button);
    }
    {
        ComboBox::Params params(GetAccessor(), GetUI(), GetWindowKey());
        params.fields[ComboBox::Fields::Value] = "choosedPreset";
        params.fields[ComboBox::Fields::Enumerator] = "presetNames";
        params.fields[ComboBox::Fields::MultipleValueText] = "unchoosedPresetText";
        ComboBox* comboBox = new ComboBox(params, wrappersProcessor, model, w->ToWidgetCast());
        w->AddControl(comboBox);
    }

    return w;
}

void KeyedArchiveEditor::OnCreatePropertyClicked()
{
    using namespace DAVA;
    using namespace DAVA;

    Vector<RefPtr<KeyedArchive>> archives;
    archives.reserve(nodes.size());
    for (const std::shared_ptr<PropertyNode>& node : nodes)
    {
        KeyedArchive* archive = node->field.ref.GetValue().Cast<KeyedArchive*>();
        archives.push_back(RefPtr<KeyedArchive>::ConstructWithRetain(archive));
    }

    if (widget == nullptr)
    {
        widget = new AddKeyedArchiveItemWidget(GetAccessor(), GetUI(), GetWindowKey(), std::move(archives), lastAddedType);
        widget->commitAddPropperty.Connect(this, &KeyedArchiveEditor::AddProperty);
    }

    widget->Show();

    QWidget* thisWidget = editorWidget->ToWidgetCast();
    QPoint topLeft = thisWidget->mapToGlobal(QPoint(0, 0));

    QRect wRect = widget->geometry();
    QPoint wPos = QPoint(topLeft.x() - wRect.width(), topLeft.y());

    widget->move(wPos);
}

void KeyedArchiveEditor::OnCreatePresetPropertyClicked()
{
    DVASSERT(choosedPreset > -1 && choosedPreset < presetNames.size());
    AddProperty(presetNames[choosedPreset], defaultValues[choosedPreset]);
    choosedPreset = -1;
}

void KeyedArchiveEditor::AddProperty(const DAVA::String& key, const DAVA::VariantType& value)
{
    using namespace DAVA;
    using namespace DAVA;

    ModifyExtension::MultiCommandInterface cmdInterface = GetModifyInterface()->GetMultiCommandInterface(Format("Add property %s", key.c_str()), static_cast<uint32>(nodes.size()));
    for (const std::shared_ptr<PropertyNode>& node : nodes)
    {
        KeyedArchive* archive = node->field.ref.GetValue().Cast<KeyedArchive*>();
        if (archive->Count(key) == 0)
        {
            cmdInterface.Exec(std::make_unique<KeyedArchiveAddValueCommand>(archive, key, value));
        }
    }
}

bool KeyedArchiveEditor::IsPresetChoosed() const
{
    return choosedPreset > -1;
}

DAVA::int32 KeyedArchiveEditor::GetChoosedPreset() const
{
    return choosedPreset;
}

void KeyedArchiveEditor::SetChoosedPreset(DAVA::int32 choosedPreset_)
{
    DVASSERT(choosedPreset != choosedPreset_);
    choosedPreset = choosedPreset_;
    editorWidget->ForceUpdate();
}

int KeyedArchiveEditor::lastAddedType = DAVA::VariantType::TYPE_STRING;

DAVA_VIRTUAL_REFLECTION_IMPL(KeyedArchiveEditor)
{
    DAVA::ReflectionRegistrator<KeyedArchiveEditor>::Begin()
    .Field("presetNames", &KeyedArchiveEditor::presetNames)
    .Field("choosedPreset", &KeyedArchiveEditor::GetChoosedPreset, &KeyedArchiveEditor::SetChoosedPreset)
    .Field("unchoosedPresetText", [](KeyedArchiveEditor* v) { return "Choose preset for Add"; }, nullptr)
    .Field("isPresetChoosed", &KeyedArchiveEditor::IsPresetChoosed, nullptr)
    .Field("createPresetButtonIcon", [](KeyedArchiveEditor* v) { return DAVA::SharedIcon(":/QtIcons/add_green.png"); }, nullptr)
    .Method("createPresetValue", &KeyedArchiveEditor::OnCreatePresetPropertyClicked)
    .Field("createPropertyButtonIcon", [](KeyedArchiveEditor* v) { return DAVA::SharedIcon(":/QtIcons/keyplus.png"); }, nullptr)
    .Field("buttonIconSize", [](KeyedArchiveEditor* v) { return DAVA::BaseComponentValue::toolButtonIconSize; }, nullptr)
    .Field("buttonAutoRise", [](KeyedArchiveEditor* v) { return false; }, nullptr)
    .Method("createPropertyValue", &KeyedArchiveEditor::OnCreatePropertyClicked)
    .End();
}

KeyedArchiveComboPresetEditor::KeyedArchiveComboPresetEditor(const DAVA::Vector<DAVA::Any>& values)
    : allowedValues(values)
{
}

DAVA::Any KeyedArchiveComboPresetEditor::GetMultipleValue() const
{
    return DAVA::Any();
}

bool KeyedArchiveComboPresetEditor::IsValidValueToSet(const DAVA::Any& newValue, const DAVA::Any& currentValue) const
{
    if (newValue.IsEmpty())
    {
        return false;
    }

    if (currentValue.IsEmpty())
    {
        return true;
    }

    int newIntValue = newValue.Cast<DAVA::int32>();
    int currentIntValue = currentValue.Cast<DAVA::int32>();

    return newIntValue != currentIntValue;
}

DAVA::ControlProxy* KeyedArchiveComboPresetEditor::CreateEditorWidget(QWidget* parent, const DAVA::Reflection& model, DAVA::DataWrappersProcessor* wrappersProcessor)
{
    DAVA::ComboBox::Params params(GetAccessor(), GetUI(), GetWindowKey());
    params.fields[DAVA::ComboBox::Fields::Value] = "value";
    params.fields[DAVA::ComboBox::Fields::Enumerator] = "values";
    params.fields[DAVA::ComboBox::Fields::IsReadOnly] = readOnlyFieldName;

    return new DAVA::ComboBox(params, wrappersProcessor, model, parent);
}

DAVA::Any KeyedArchiveComboPresetEditor::GetValueAny() const
{
    return GetValue();
}

void KeyedArchiveComboPresetEditor::SetValueAny(const DAVA::Any& newValue)
{
    SetValue(newValue);
}

DAVA_VIRTUAL_REFLECTION_IMPL(KeyedArchiveComboPresetEditor)
{
    DAVA::ReflectionRegistrator<KeyedArchiveComboPresetEditor>::Begin()
    .Field("value", &KeyedArchiveComboPresetEditor::GetValueAny, &KeyedArchiveComboPresetEditor::SetValueAny)
    .Field("values", &KeyedArchiveComboPresetEditor::allowedValues)
    .End();
}

} // namespace PropertyPanel

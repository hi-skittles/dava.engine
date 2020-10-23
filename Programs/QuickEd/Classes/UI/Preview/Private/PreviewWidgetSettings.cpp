#include "UI/Preview/PreviewWidgetSettings.h"

#include <TArc/Controls/PropertyPanel/PropertyPanelMeta.h>
#include <TArc/Core/ContextAccessor.h>
#include <TArc/DataProcessing/PropertiesHolder.h>
#include <TArc/Utils/Utils.h>

#include <Math/Color.h>
#include <Reflection/Reflection.h>

const DAVA::Color PreviewWidgetSettings::defaultBackgroundColor0 = DAVA::Color(0.0f, 0.0f, 0.0f, 0.5f);
const DAVA::Color PreviewWidgetSettings::defaultBackgroundColor1 = DAVA::Color(0.242f, 0.242f, 0.242f, 1.0f);
const DAVA::Color PreviewWidgetSettings::defaultBackgroundColor2 = DAVA::Color(0.159f, 0.159f, 0.159f, 1.0f);

namespace PreviewWidgetSettingsDetail
{
using namespace DAVA;

void LoadIntoReflection(Reflection::Field& field, const PropertiesItem& node)
{
    Vector<Reflection::Field> fields = field.ref.GetFields();
    if (fields.empty())
    {
        Any value = node.Get(field.key.Cast<String>(), field.ref.GetValue(), field.ref.GetValueType());
        if (value.IsEmpty() == false)
        {
            field.ref.SetValueWithCast(value);
        }
    }
    else
    {
        for (Reflection::Field& f : fields)
        {
            PropertiesItem fieldItem = node.CreateSubHolder(f.key.Cast<String>());
            LoadIntoReflection(f, fieldItem);
        }
    }
}
} // namespace PreviewWidgetSettingsDetail

struct PreviewWidgetSettingsV0
{
    DAVA::Color backgroundColor0 = PreviewWidgetSettings::defaultBackgroundColor0;
    DAVA::Color backgroundColor1 = PreviewWidgetSettings::defaultBackgroundColor1;
    DAVA::Color backgroundColor2 = PreviewWidgetSettings::defaultBackgroundColor2;
    DAVA::uint32 backgroundColorIndex = 0;

    DAVA_REFLECTION(PreviewWidgetSettingsV0)
    {
        DAVA::ReflectionRegistrator<PreviewWidgetSettingsV0>::Begin()
        .ConstructorByPointer()
        .Field("backgroundColor0", &PreviewWidgetSettingsV0::backgroundColor0)
        .Field("backgroundColor1", &PreviewWidgetSettingsV0::backgroundColor1)
        .Field("backgroundColor2", &PreviewWidgetSettingsV0::backgroundColor2)
        .Field("backgroundColorIndex", &PreviewWidgetSettingsV0::backgroundColorIndex)
        .End();
    }
};

class AddBgrColorCommand : public DAVA::Command
{
public:
    AddBgrColorCommand(DAVA::ContextAccessor* accessor)
        : accessor(accessor)
    {
    }

private:
    void Redo()
    {
        PreviewWidgetSettings* settings = accessor->GetGlobalContext()->GetData<PreviewWidgetSettings>();
        settings->backgroundColors.emplace_back(DAVA::Color::White);
    }
    void Undo()
    {
        PreviewWidgetSettings* settings = accessor->GetGlobalContext()->GetData<PreviewWidgetSettings>();
        settings->backgroundColors.pop_back();
    }

private:
    DAVA::ContextAccessor* accessor = nullptr;
};

class AddBgrColorProducer : public DAVA::M::CommandProducer
{
public:
    AddBgrColorProducer() = default;
    bool IsApplyable(const std::shared_ptr<DAVA::PropertyNode>& node) const override
    {
        return true;
    }

    Info GetInfo() const override
    {
        Info info;
        info.icon = DAVA::SharedIcon(":/Icons/add.png");
        info.tooltip = "add background color";
        info.description = "add background color";
        return info;
    }

    std::unique_ptr<DAVA::Command> CreateCommand(const std::shared_ptr<DAVA::PropertyNode>& node, const Params& params) const override
    {
        return std::make_unique<AddBgrColorCommand>(params.accessor);
    }
};

class AddBgrColorFeature : public DAVA::M::CommandProducerHolder
{
public:
    AddBgrColorFeature()
    {
        AddCommandProducer(std::make_shared<AddBgrColorProducer>());
    }
};

DAVA_VIRTUAL_REFLECTION_IMPL(PreviewWidgetSettings)
{
    DAVA::ReflectionRegistrator<PreviewWidgetSettings>::Begin()[DAVA::M::DisplayName("Preview widget"), DAVA::M::SettingsSortKey(70)]
    .ConstructorByPointer()
    .Field("backgroundColors", &PreviewWidgetSettings::backgroundColors)[DAVA::M::DisplayName("Background colors"), AddBgrColorFeature()]
    .Field("backgroundColorIndex", &PreviewWidgetSettings::backgroundColorIndex)[DAVA::M::HiddenField(), DAVA::M::ForceResetToDefault()]
    .Field("version", &PreviewWidgetSettings::version)[DAVA::M::HiddenField()]
    .End();
}

void PreviewWidgetSettings::Load(const DAVA::PropertiesItem& settingsItem)
{
    using namespace DAVA;

    using namespace PreviewWidgetSettingsDetail;

    Reflection settingsReflection = Reflection::Create(ReflectedObject(this));

    {
        String versionFieldName = "version";
        Reflection versionRef = settingsReflection.GetField(versionFieldName);
        PropertiesItem versionItem = settingsItem.CreateSubHolder(versionFieldName);
        uint32 versionDefaultValue = 0;

        Any value = versionItem.Get(versionFieldName, versionDefaultValue, versionRef.GetValueType());
        if (value.CanCast<uint32>())
        {
            version = value.Cast<uint32>(0);
        }
    }

    switch (version)
    {
    case 0:
        LoadVersion0(settingsItem);
        break;
    case 1:
        LoadVersion1(settingsItem, settingsReflection);
        break;
    default:
        LoadDefaultValues();
        break;
    }

    version = 1;
}

void PreviewWidgetSettings::Save(DAVA::PropertiesItem& settingsItem) const
{
    using namespace DAVA;

    Reflection::Field rootField;
    rootField.ref = Reflection::Create(ReflectedObject(this));
    rootField.key = String("Root");

    auto SaveField = [rootField, &settingsItem](String fieldName)
    {
        Reflection::Field field;
        field.key = fieldName;
        field.ref = rootField.ref.GetField(fieldName);
        PropertiesItem item = settingsItem.CreateSubHolder(fieldName);

        item.Set(fieldName, field.ref.GetValue());
    };

    SaveField("backgroundColors");
    SaveField("backgroundColorIndex");
    SaveField("version");
}

void PreviewWidgetSettings::LoadVersion0(const DAVA::PropertiesItem& settingsNode)
{
    using namespace DAVA;

    PreviewWidgetSettingsV0 settingsV0;

    Reflection::Field rootField;
    rootField.ref = Reflection::Create(ReflectedObject(&settingsV0));
    rootField.key = String("Root");
    PreviewWidgetSettingsDetail::LoadIntoReflection(rootField, settingsNode);

    backgroundColors.resize(3);
    backgroundColors[0] = settingsV0.backgroundColor0;
    backgroundColors[1] = settingsV0.backgroundColor1;
    backgroundColors[2] = settingsV0.backgroundColor2;
    backgroundColorIndex = settingsV0.backgroundColorIndex;
}

void PreviewWidgetSettings::LoadVersion1(const DAVA::PropertiesItem& settingsItem, DAVA::Reflection& settingsReflection)
{
    using namespace DAVA;

    {
        String colorsFieldName = "backgroundColors";
        Reflection::Field colorsField;
        colorsField.key = colorsFieldName;
        colorsField.ref = settingsReflection.GetField(colorsFieldName);
        PropertiesItem colorsItem = settingsItem.CreateSubHolder(colorsFieldName);

        Any value = colorsItem.Get(colorsFieldName, colorsField.ref.GetValue(), colorsField.ref.GetValueType());
        colorsField.ref.SetValueWithCast(value);
    }

    {
        String indexFieldName = "backgroundColorIndex";
        Reflection::Field indexField;
        indexField.key = indexFieldName;
        indexField.ref = settingsReflection.GetField(indexFieldName);
        PropertiesItem indexItem = settingsItem.CreateSubHolder(indexFieldName);

        Any value = indexItem.Get(indexFieldName, indexField.ref.GetValue(), indexField.ref.GetValueType());
        if (value.IsEmpty() == false)
        {
            indexField.ref.SetValueWithCast(value);
        }
    }
}

void PreviewWidgetSettings::LoadDefaultValues()
{
    using namespace PreviewWidgetSettingsDetail;

    backgroundColors.resize(3);
    backgroundColors[0] = defaultBackgroundColor0;
    backgroundColors[1] = defaultBackgroundColor1;
    backgroundColors[2] = defaultBackgroundColor2;
    backgroundColorIndex = 0;
    version = 1;
}

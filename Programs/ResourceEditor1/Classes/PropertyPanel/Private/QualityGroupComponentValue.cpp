#include "Classes/PropertyPanel/Private/QualityGroupComponentValue.h"

#include <TArc/Controls/PropertyPanel/PropertyModelExtensions.h>
#include <TArc/Controls/ControlProxy.h>
#include <TArc/Controls/QtBoxLayouts.h>
#include <TArc/Controls/ListView.h>
#include <TArc/Controls/CheckBox.h>
#include <TArc/Controls/ComboBox.h>
#include <TArc/Controls/CommonStrings.h>

#include <Scene3D/Systems/QualitySettingsSystem.h>
#include <Scene3D/Components/QualitySettingsComponent.h>

#include <QWidget>
#include <QSplitter>
#include <QLabel>

namespace QualityGroupComponentValueDetail
{
class Control : private QWidget, public DAVA::TArc::ControlProxy
{
public:
    Control(QWidget* parent, const DAVA::Reflection& model, DAVA::TArc::DataWrappersProcessor* wrappersProcessor,
            DAVA::TArc::ContextAccessor* accessor, DAVA::TArc::UI* ui, const DAVA::TArc::WindowKey& wndKey)
        : QWidget(parent)
    {
        using namespace DAVA::TArc;
        QtVBoxLayout* layout = new QtVBoxLayout(this);
        layout->setSpacing(0);
        layout->setMargin(0);

        {
            QtHBoxLayout* filterLayout = new QtHBoxLayout();
            filterLayout->setSpacing(2);
            filterLayout->setContentsMargins(0, 2, 0, 2);
            filterLayout->addWidget(new QLabel("Filter by Type", this));

            {
                CheckBox::Params params(accessor, ui, wndKey);
                params.fields[CheckBox::Fields::Checked] = "filterByType";
                params.fields[CheckBox::Fields::IsReadOnly] = BaseComponentValue::readOnlyFieldName;
                CheckBox* filterByTypeEditor = new CheckBox(params, wrappersProcessor, model, this);
                filterLayout->AddControl(filterByTypeEditor);
                subControls.push_back(filterByTypeEditor);
            }

            {
                ComboBox::Params params(accessor, ui, wndKey);
                params.fields[ComboBox::Fields::Value] = "modelType";
                params.fields[ComboBox::Fields::IsReadOnly] = "isModelTypeReadOnly";
                params.fields[ComboBox::Fields::Enumerator] = "modelTypes";
                ComboBox* modelTypeEditor = new ComboBox(params, wrappersProcessor, model, this);
                QSizePolicy policy = modelTypeEditor->ToWidgetCast()->sizePolicy();
                policy.setHorizontalPolicy(QSizePolicy::Expanding);
                modelTypeEditor->ToWidgetCast()->setSizePolicy(policy);
                filterLayout->AddControl(modelTypeEditor);
                subControls.push_back(modelTypeEditor);
            }

            layout->addLayout(filterLayout);
        }

        QtHBoxLayout* groupQualityLayout = new QtHBoxLayout();
        groupQualityLayout->setSpacing(2);
        groupQualityLayout->setContentsMargins(0, 2, 0, 2);

        {
            QtVBoxLayout* groupLayout = new QtVBoxLayout();
            groupLayout->setSpacing(1);

            groupLayout->addWidget(new QLabel("Group", this), 0, Qt::AlignHCenter);

            ComboBox::Params params(accessor, ui, wndKey);
            params.fields[ComboBox::Fields::Value] = "group";
            params.fields[ComboBox::Fields::Enumerator] = "groups";
            params.fields[ComboBox::Fields::IsReadOnly] = "isGroupReadOnly";
            ComboBox* groupComboBox = new ComboBox(params, wrappersProcessor, model, this);
            groupLayout->AddControl(groupComboBox, 1);
            subControls.push_back(groupComboBox);

            groupQualityLayout->addLayout(groupLayout);
        }

        {
            QtVBoxLayout* qualityLayout = new QtVBoxLayout();
            qualityLayout->setSpacing(1);

            qualityLayout->addWidget(new QLabel("Quality", this), 0, Qt::AlignHCenter);
            ComboBox::Params params(accessor, ui, wndKey);
            params.fields[ComboBox::Fields::Value] = "quality";
            params.fields[ComboBox::Fields::Enumerator] = "qualities";
            params.fields[ComboBox::Fields::IsReadOnly] = "isQualityReadOnly";
            ComboBox* qualityComboBox = new ComboBox(params, wrappersProcessor, model, this);
            qualityLayout->AddControl(qualityComboBox, 1);
            subControls.push_back(qualityComboBox);

            groupQualityLayout->addLayout(qualityLayout);
        }

        layout->addLayout(groupQualityLayout);
    }

    void ForceUpdate() override
    {
        std::for_each(subControls.begin(), subControls.end(), [](ControlProxy* control)
                      {
                          control->ForceUpdate();
                      });
    }

    virtual void TearDown() override
    {
        std::for_each(subControls.begin(), subControls.end(), [](ControlProxy* control)
                      {
                          control->TearDown();
                      });
    }

    QWidget* ToWidgetCast() override
    {
        return this;
    }

private:
    DAVA::Vector<DAVA::TArc::ControlProxy*> subControls;
};
}

QualityGroupComponentValue::QualityGroupComponentValue()
{
    using namespace DAVA;
    using namespace QualityGroupComponentValueDetail;

    QualitySettingsSystem* system = QualitySettingsSystem::Instance();

    int32 filterCount = system->GetOptionsCount();
    modelTypes.reserve(filterCount + 1);
    modelTypesWithDifferent.reserve(filterCount + 2);

    DAVA::FastName multiValue(DAVA::TArc::MultipleValuesString);
    DAVA::FastName undefinedValue(DAVA::TArc::UndefinedString);

    modelTypes.push_back(undefinedValue);
    modelTypesWithDifferent.push_back(multiValue);
    modelTypesWithDifferent.push_back(undefinedValue);

    for (int32 i = 0; i < filterCount; ++i)
    {
        FastName modelFilterName = system->GetOptionName(i);
        modelTypes.push_back(modelFilterName);
        modelTypesWithDifferent.push_back(modelFilterName);
    }

    size_t groupCount = system->GetMaterialQualityGroupCount();
    groups.reserve(groupCount + 1);
    groupsWithDifferent.reserve(groupCount + 2);

    groupsWithDifferent.push_back(multiValue);
    groupsWithDifferent.push_back(undefinedValue);
    groups.push_back(undefinedValue);

    using TQualityNode = std::pair<Vector<FastName>, Vector<FastName>>;
    TQualityNode& differentValueNode = qualities[multiValue];
    differentValueNode.first.push_back(multiValue);
    differentValueNode.second.push_back(multiValue);

    TQualityNode& undefinedNode = qualities[undefinedValue];
    undefinedNode.first.push_back(undefinedValue);
    undefinedNode.second.push_back(undefinedValue);

    for (size_t group = 0; group < system->GetMaterialQualityGroupCount(); ++group)
    {
        FastName groupName = system->GetMaterialQualityGroupName(group);
        groups.push_back(groupName);
        groupsWithDifferent.push_back(groupName);

        TQualityNode& node = qualities[groupName];
        node.first.push_back(undefinedValue);
        node.second.push_back(multiValue);
        node.second.push_back(undefinedValue);

        for (size_t quality = 0; quality < system->GetMaterialQualityCount(groupName); ++quality)
        {
            FastName qualityName = system->GetMaterialQualityName(groupName, quality);
            node.first.push_back(qualityName);
            node.second.push_back(qualityName);
        }
    }
}

bool QualityGroupComponentValue::RepaintOnUpdateRequire() const
{
    return true;
}

DAVA::Any QualityGroupComponentValue::GetMultipleValue() const
{
    return DAVA::Any();
}

bool QualityGroupComponentValue::IsValidValueToSet(const DAVA::Any& newValue, const DAVA::Any& currentValue) const
{
    return false;
}

DAVA::TArc::ControlProxy* QualityGroupComponentValue::CreateEditorWidget(QWidget* parent, const DAVA::Reflection& model, DAVA::TArc::DataWrappersProcessor* wrappersProcessor)
{
    return new QualityGroupComponentValueDetail::Control(parent, model, wrappersProcessor, GetAccessor(), GetUI(), GetWindowKey());
}

bool QualityGroupComponentValue::IsSpannedControl() const
{
    return true;
}

DAVA::Any QualityGroupComponentValue::IsFilterByType() const
{
    DAVA::QualitySettingsComponent* component = nodes.front()->cachedValue.Cast<DAVA::QualitySettingsComponent*>();
    bool filterByType = component->GetFilterByType();
    for (size_t i = 1; i < nodes.size(); ++i)
    {
        component = nodes[i]->cachedValue.Cast<DAVA::QualitySettingsComponent*>();
        if (filterByType != component->GetFilterByType())
        {
            isDifferentFilterByType = true;
            return DAVA::Any(Qt::PartiallyChecked);
        }
    }

    return filterByType == true ? Qt::Checked : Qt::Unchecked;
}

void QualityGroupComponentValue::SetFilterByType(const DAVA::Any& filtrate)
{
    using namespace DAVA::TArc;
    Qt::CheckState state = filtrate.Cast<Qt::CheckState>();
    if (state == Qt::PartiallyChecked)
    {
        return;
    }

    if (filtrate == IsFilterByType())
    {
        return;
    }

    ModifyExtension::MultiCommandInterface cmdInterface = GetModifyInterface()->GetMultiCommandInterface("Filter by type change", static_cast<DAVA::uint32>(nodes.size()));
    for (std::shared_ptr<PropertyNode>& node : nodes)
    {
        DAVA::Reflection componentR = node->field.ref;
        DAVA::Reflection::Field filterByTypeField;
        filterByTypeField.key = "filterByType";
        filterByTypeField.ref = componentR.GetField(filterByTypeField.key);
        cmdInterface.ProduceCommand(filterByTypeField, filtrate);
    }

    isDifferentFilterByType = false;
    editorWidget->ForceUpdate();
}

size_t QualityGroupComponentValue::GetModelTypeIndex() const
{
    DAVA::QualitySettingsComponent* component = nodes.front()->cachedValue.Cast<DAVA::QualitySettingsComponent*>();
    DAVA::FastName modelType = component->GetModelType();
    for (size_t i = 1; i < nodes.size(); ++i)
    {
        component = nodes[i]->cachedValue.Cast<DAVA::QualitySettingsComponent*>();
        if (modelType != component->GetModelType())
        {
            isDifferentModelTypes = true;
            return 0;
        }
    }

    isDifferentModelTypes = false;
    if (modelType.IsValid() == false)
    {
        return 0;
    }

    auto iter = std::find(modelTypes.begin(), modelTypes.end(), modelType);
    return std::distance(modelTypes.begin(), iter);
}

void QualityGroupComponentValue::SetModelTypeIndex(size_t index)
{
    using namespace DAVA::TArc;
    if (isDifferentModelTypes == true && index == 0)
    {
        return;
    }

    DAVA::FastName newModelType;
    if (isDifferentModelTypes == false)
    {
        DVASSERT(index < modelTypes.size());
        newModelType = modelTypes[index];
    }
    else
    {
        DVASSERT(index < modelTypesWithDifferent.size());
        newModelType = modelTypesWithDifferent[index];
    }

    ModifyExtension::MultiCommandInterface cmdInterface = GetModifyInterface()->GetMultiCommandInterface("Quality Group change", static_cast<DAVA::uint32>(nodes.size()));
    for (std::shared_ptr<PropertyNode>& node : nodes)
    {
        DAVA::Reflection componentR = node->field.ref;
        DAVA::Reflection::Field modelTypeField;
        modelTypeField.key = "modelType";
        modelTypeField.ref = componentR.GetField(modelTypeField.key);
        cmdInterface.ProduceCommand(modelTypeField, newModelType);
    }

    isDifferentModelTypes = false;
    editorWidget->ForceUpdate();
}

bool QualityGroupComponentValue::IsModelTypeReadOnly() const
{
    bool isReadOnly = IsReadOnly();
    Qt::CheckState filterByType = IsFilterByType().Cast<Qt::CheckState>();
    return isReadOnly || filterByType != Qt::Checked;
}

size_t QualityGroupComponentValue::GetGroupIndex() const
{
    DAVA::QualitySettingsComponent* component = nodes.front()->cachedValue.Cast<DAVA::QualitySettingsComponent*>();
    DAVA::FastName group = component->GetRequiredGroup();
    for (size_t i = 1; i < nodes.size(); ++i)
    {
        component = nodes[i]->cachedValue.Cast<DAVA::QualitySettingsComponent*>();
        if (group != component->GetRequiredGroup())
        {
            isDifferentGroups = true;
            return 0;
        }
    }

    isDifferentGroups = false;
    if (group.IsValid() == false)
    {
        return 0;
    }

    auto iter = std::find(groups.begin(), groups.end(), group);
    return std::distance(groups.begin(), iter);
}

void QualityGroupComponentValue::SetGroupIndex(size_t index)
{
    using namespace DAVA::TArc;
    if (isDifferentGroups == true && index == 0)
    {
        return;
    }

    DAVA::FastName newGroup;
    if (isDifferentGroups == false)
    {
        DVASSERT(index < groups.size());
        newGroup = groups[index];
    }
    else
    {
        DVASSERT(index < groupsWithDifferent.size());
        newGroup = groupsWithDifferent[index];
    }

    ModifyExtension::MultiCommandInterface cmdInterface = GetModifyInterface()->GetMultiCommandInterface("Quality Group change", static_cast<DAVA::uint32>(nodes.size()));
    for (std::shared_ptr<PropertyNode>& node : nodes)
    {
        DAVA::Reflection componentR = node->field.ref;
        DAVA::Reflection::Field requestedGroup;
        requestedGroup.key = "requiredGroup";
        requestedGroup.ref = componentR.GetField(requestedGroup.key);

        DAVA::Reflection::Field requestedQuality;
        requestedQuality.key = "requiredQuality";
        requestedQuality.ref = componentR.GetField(requestedQuality.key);
        cmdInterface.ProduceCommand(requestedGroup, newGroup);
        cmdInterface.ProduceCommand(requestedQuality, DAVA::FastName());
    }

    isDifferentGroups = false;
    editorWidget->ForceUpdate();
}

size_t QualityGroupComponentValue::GetQualityIndex() const
{
    if (isDifferentGroups == true)
    {
        isDifferentQualities = true;
        return 0;
    }

    size_t groupIndex = GetGroupIndex();
    if (groupIndex == 0)
    {
        isDifferentQualities = false;
        return 0;
    }

    DAVA::QualitySettingsComponent* component = nodes.front()->cachedValue.Cast<DAVA::QualitySettingsComponent*>();
    DAVA::FastName quality = component->GetRequiredQuality();
    for (size_t i = 1; i < nodes.size(); ++i)
    {
        component = nodes[i]->cachedValue.Cast<DAVA::QualitySettingsComponent*>();
        if (quality != component->GetRequiredQuality())
        {
            isDifferentQualities = true;
            return 0;
        }
    }

    isDifferentQualities = false;
    if (quality.IsValid() == false)
    {
        return 0;
    }

    auto groupQualities = qualities.find(component->GetRequiredGroup());
    DVASSERT(groupQualities != qualities.end());

    const DAVA::Vector<DAVA::FastName>& currentQualities = groupQualities->second.first;

    auto qualitiIter = std::find(currentQualities.begin(), currentQualities.end(), quality);
    return std::distance(currentQualities.begin(), qualitiIter);
}

void QualityGroupComponentValue::SetQualityIndex(size_t index)
{
    using namespace DAVA::TArc;
    if (isDifferentQualities == true && index == 0)
    {
        return;
    }

    DVASSERT(isDifferentGroups == false);

    DAVA::QualitySettingsComponent* component = nodes.front()->cachedValue.Cast<DAVA::QualitySettingsComponent*>();
    auto groupQualities = qualities.find(component->GetRequiredGroup());
    DVASSERT(groupQualities != qualities.end());

    DAVA::FastName newQuality;
    if (isDifferentQualities == false)
    {
        DVASSERT(index < groupQualities->second.first.size());
        newQuality = groupQualities->second.first[index];
    }
    else
    {
        DVASSERT(index < groupQualities->second.second.size());
        newQuality = groupQualities->second.second[index];
    }

    ModifyExtension::MultiCommandInterface cmdInterface = GetModifyInterface()->GetMultiCommandInterface("Quality Group change", static_cast<DAVA::uint32>(nodes.size()));
    for (std::shared_ptr<PropertyNode>& node : nodes)
    {
        DAVA::Reflection componentR = node->field.ref;
        DAVA::Reflection::Field requestedQuality;
        requestedQuality.key = "requiredQuality";
        requestedQuality.ref = componentR.GetField(requestedQuality.key);
        cmdInterface.ProduceCommand(requestedQuality, newQuality);
    }

    isDifferentQualities = false;
    editorWidget->ForceUpdate();
}

bool QualityGroupComponentValue::IsGroupReadOnly() const
{
    bool isReadOnly = IsReadOnly();
    Qt::CheckState filterByType = IsFilterByType().Cast<Qt::CheckState>();
    return isReadOnly || filterByType != Qt::Unchecked;
}

bool QualityGroupComponentValue::IsQualityReadOnly() const
{
    size_t groupIndex = GetGroupIndex();
    return groupIndex == 0 || IsGroupReadOnly() == true;
}

const DAVA::Vector<DAVA::FastName>& QualityGroupComponentValue::GetFilters() const
{
    if (isDifferentModelTypes == true)
        return modelTypesWithDifferent;
    else
        return modelTypes;
}

const DAVA::Vector<DAVA::FastName>& QualityGroupComponentValue::GetGroups() const
{
    if (isDifferentGroups == true)
        return groupsWithDifferent;
    else
        return groups;
}

const DAVA::Vector<DAVA::FastName>& QualityGroupComponentValue::GetQualities() const
{
    using namespace QualityGroupComponentValueDetail;

    const DAVA::QualitySettingsComponent* qualityComponent = nodes.front()->cachedValue.Cast<DAVA::QualitySettingsComponent*>();
    DAVA::FastName group = qualityComponent->GetRequiredGroup();
    if (isDifferentGroups == true)
    {
        group = DAVA::FastName(DAVA::TArc::MultipleValuesString);
    }
    else if (group.IsValid() == false)
    {
        group = DAVA::FastName(DAVA::TArc::UndefinedString);
    }

    auto iter = qualities.find(group);
    DVASSERT(iter != qualities.end());

    if (isDifferentQualities == true)
    {
        return iter->second.second;
    }
    else
    {
        return iter->second.first;
    }
}

DAVA_VIRTUAL_REFLECTION_IMPL(QualityGroupComponentValue)
{
    DAVA::ReflectionRegistrator<QualityGroupComponentValue>::Begin()
    .Field("filterByType", &QualityGroupComponentValue::IsFilterByType, &QualityGroupComponentValue::SetFilterByType)
    .Field("modelType", &QualityGroupComponentValue::GetModelTypeIndex, &QualityGroupComponentValue::SetModelTypeIndex)
    .Field("isModelTypeReadOnly", &QualityGroupComponentValue::IsModelTypeReadOnly, nullptr)
    .Field("group", &QualityGroupComponentValue::GetGroupIndex, &QualityGroupComponentValue::SetGroupIndex)
    .Field("isGroupReadOnly", &QualityGroupComponentValue::IsGroupReadOnly, nullptr)
    .Field("quality", &QualityGroupComponentValue::GetQualityIndex, &QualityGroupComponentValue::SetQualityIndex)
    .Field("isQualityReadOnly", &QualityGroupComponentValue::IsQualityReadOnly, nullptr)
    .Field("modelTypes", &QualityGroupComponentValue::GetFilters, nullptr)
    .Field("groups", &QualityGroupComponentValue::GetGroups, nullptr)
    .Field("qualities", &QualityGroupComponentValue::GetQualities, nullptr)
    .End();
}

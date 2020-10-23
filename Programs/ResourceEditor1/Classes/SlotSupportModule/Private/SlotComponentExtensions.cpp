#include "Classes/SlotSupportModule/Private/SlotComponentExtensions.h"
#include "Classes/PropertyPanel/PropertyPanelCommon.h"
#include "Classes/Commands2/SlotCommands.h"
#include "Classes/Commands2/AddComponentCommand.h"
#include "Classes/Commands2/SetFieldValueCommand.h"
#include "Classes/Qt/Scene/SceneEditor2.h"
#include "Classes/SceneManager/SceneData.h"
#include "Classes/SlotSupportModule/Private/EditorSlotSystem.h"
#include "Classes/SlotSupportModule/Private/SlotTemplatesData.h"
#include "Classes/SlotSupportModule/SlotSystemSettings.h"

#include <TArc/Controls/CommonStrings.h>
#include <TArc/Controls/ComboBox.h>
#include <TArc/Controls/LineEdit.h>
#include <TArc/Controls/FilePathEdit.h>
#include <TArc/Controls/Widget.h>
#include <TArc/Controls/ReflectedButton.h>
#include <TArc/Controls/ListView.h>
#include <TArc/Controls/PopupLineEdit.h>
#include <TArc/Utils/ReflectionHelpers.h>
#include <TArc/Utils/Utils.h>
#include <TArc/Controls/PropertyPanel/PropertyPanelMeta.h>
#include <TArc/Controls/PropertyPanel/Private/TextComponentValue.h>

#include <Scene3D/Systems/SlotSystem.h>
#include <Scene3D/Components/SlotComponent.h>
#include <Scene3D/SceneFile/VersionInfo.h>
#include <Engine/PlatformApiQt.h>
#include <FileSystem/FilePath.h>
#include <Base/BaseTypes.h>
#include <Base/Any.h>

#include <QHBoxLayout>
#include <QMimeData>
#include <QApplication>
#include <QClipboard>

namespace PropertyPanel
{
class BaseSlotComponentValue : public DAVA::TArc::BaseComponentValue
{
protected:
    void ForEachSlotComponent(const DAVA::Function<bool(DAVA::SlotComponent*, bool)>& fn) const
    {
        for (size_t i = 0; i < nodes.size(); ++i)
        {
            std::shared_ptr<DAVA::TArc::PropertyNode> node = nodes[i];
            DAVA::SlotComponent* component = node->cachedValue.Cast<DAVA::SlotComponent*>(nullptr);
            DVASSERT(component != nullptr);
            if (fn(component, i == 0) == false)
            {
                break;
            }
        }
    }

    DAVA_VIRTUAL_REFLECTION_IN_PLACE(BaseSlotComponentValue, DAVA::TArc::BaseComponentValue)
    {
        DAVA::ReflectionRegistrator<BaseSlotComponentValue>::Begin()
        .End();
    }
};

class SlotTypeFiltersComponentValue : public BaseSlotComponentValue
{
public:
    DAVA::Any GetMultipleValue() const override
    {
        return DAVA::Any();
    }

    bool IsValidValueToSet(const DAVA::Any& newValue, const DAVA::Any& currentValue) const override
    {
        return false;
    }

    DAVA::TArc::ControlProxy* CreateEditorWidget(QWidget* parent, const DAVA::Reflection& model, DAVA::TArc::DataWrappersProcessor* wrappersProcessor) override
    {
        using namespace DAVA::TArc;

        Widget* w = new Widget(parent);
        QHBoxLayout* layout = new QHBoxLayout();
        layout->setContentsMargins(0, 2, 2, 2);
        w->SetLayout(layout);

        ListView::Params params(GetAccessor(), GetUI(), GetWindowKey());
        params.fields[ListView::Fields::ValueList] = "filtersList";
        params.fields[ListView::Fields::CurrentValue] = "currentFilter";

        ListView* filtersControl = new ListView(params, wrappersProcessor, model, parent);
        filtersControl->ToWidgetCast()->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
        w->AddControl(filtersControl);

        Widget* buttonsBar = new Widget(w->ToWidgetCast());
        buttonsBar->SetLayout(new QVBoxLayout());
        w->AddControl(buttonsBar);

        {
            ReflectedButton::Params params(GetAccessor(), GetUI(), GetWindowKey());
            params.fields[ReflectedButton::Fields::AutoRaise] = "autoRise";
            params.fields[ReflectedButton::Fields::Icon] = "addButtonIcon";
            params.fields[ReflectedButton::Fields::Tooltip] = "addButtonTooltip";
            params.fields[ReflectedButton::Fields::Clicked] = "addTypeFilter";
            params.fields[ReflectedButton::Fields::Enabled] = "addButtonEnabled";
            ReflectedButton* addButton = new ReflectedButton(params, wrappersProcessor, model, w->ToWidgetCast());
            buttonsBar->AddControl(addButton);
        }

        {
            ReflectedButton::Params params(GetAccessor(), GetUI(), GetWindowKey());
            params.fields[ReflectedButton::Fields::AutoRaise] = "autoRise";
            params.fields[ReflectedButton::Fields::Icon] = "removeButtonIcon";
            params.fields[ReflectedButton::Fields::Tooltip] = "removeButtonTooltip";
            params.fields[ReflectedButton::Fields::Enabled] = "removeButtonEnabled";
            params.fields[ReflectedButton::Fields::Clicked] = "removeTypeFilter";
            ReflectedButton* addButton = new ReflectedButton(params, wrappersProcessor, model, w->ToWidgetCast());
            buttonsBar->AddControl(addButton);
        }

        return w;
    }

private:
    const DAVA::Set<DAVA::String>& GetTypeFilters() const
    {
        DAVA::Set<DAVA::FastName> intersection;
        ForEachSlotComponent([&](DAVA::SlotComponent* component, bool isFirst) {
            DAVA::Set<DAVA::FastName> currentIntersection;
            for (DAVA::uint32 i = 0; i < component->GetTypeFiltersCount(); ++i)
            {
                DAVA::FastName filter = component->GetTypeFilter(i);
                if (isFirst == true || intersection.count(filter) > 0)
                {
                    currentIntersection.insert(component->GetTypeFilter(i));
                }
            }

            std::swap(intersection, currentIntersection);
            return true;
        });

        filters.clear();
        std::transform(intersection.begin(), intersection.end(), std::inserter(filters, filters.end()), DAVA::Bind(&DAVA::FastName::c_str, DAVA::_1));
        return filters;
    }

    void AddTypeFilter()
    {
        using namespace DAVA::TArc;

        LineEdit::Params params(GetAccessor(), GetUI(), GetWindowKey());
        params.fields[LineEdit::Fields::Text] = "addFilterPopupText";
        params.fields[LineEdit::Fields::PlaceHolder] = "filterEditPlaceholder";

        DAVA::Reflection popupModel = DAVA::Reflection::Create(DAVA::ReflectedObject(this));
        DAVA::TArc::PopupLineEdit* popupLineEdit = new DAVA::TArc::PopupLineEdit(params, GetAccessor(), popupModel, realWidget);
        popupLineEdit->Show(realWidget->parentWidget()->mapToGlobal(realWidget->geometry().topLeft()));
    }

    void RemoveTypeFilter()
    {
        DAVA::FastName filterToRemove(currentFilter);
        DAVA::String descr = DAVA::Format("Remove type filter: %s", currentFilter.c_str());
        DAVA::TArc::ModifyExtension::MultiCommandInterface cmdInterface = GetModifyInterface()->GetMultiCommandInterface(descr, static_cast<DAVA::uint32>(nodes.size()));
        ForEachSlotComponent([&](DAVA::SlotComponent* component, bool isFirst) {
            cmdInterface.Exec(std::make_unique<SlotTypeFilterEdit>(component, filterToRemove, false));
            return true;
        });

        ForceUpdate();
    }

    DAVA::String GetPopupText() const
    {
        return "";
    }

    void SetPopupText(const DAVA::String& filterName)
    {
        if (filterName.empty())
        {
            return;
        }

        DAVA::FastName filterToAdd(filterName);
        DAVA::String descr = DAVA::Format("Add type filter: %s", currentFilter.c_str());
        DAVA::TArc::ModifyExtension::MultiCommandInterface cmdInterface = GetModifyInterface()->GetMultiCommandInterface(descr, static_cast<DAVA::uint32>(nodes.size()));
        ForEachSlotComponent([&](DAVA::SlotComponent* component, bool isFirst) {
            for (DAVA::uint32 i = 0; i < component->GetTypeFiltersCount(); ++i)
            {
                if (component->GetTypeFilter(i) == filterToAdd)
                {
                    return true;
                }
            }
            cmdInterface.Exec(std::make_unique<SlotTypeFilterEdit>(component, filterToAdd, true));
            return true;
        });
    }

    DAVA::Any GetCurrentFilter() const
    {
        if (currentFilter.empty())
        {
            return DAVA::Any();
        }
        return currentFilter;
    }

    void SetCurrentFilter(const DAVA::Any& v)
    {
        if (v.IsEmpty())
        {
            currentFilter = DAVA::String("");
        }
        else
        {
            currentFilter = v.Cast<DAVA::String>();
        }

        ForceUpdate();
    }

    mutable DAVA::Set<DAVA::String> filters;
    DAVA::String currentFilter = DAVA::String("");

    DAVA_VIRTUAL_REFLECTION_IN_PLACE(SlotTypeFiltersComponentValue, BaseSlotComponentValue)
    {
        DAVA::ReflectionRegistrator<SlotTypeFiltersComponentValue>::Begin()
        .Field("filtersList", &SlotTypeFiltersComponentValue::GetTypeFilters, nullptr)
        .Field("currentFilter", &SlotTypeFiltersComponentValue::GetCurrentFilter, &SlotTypeFiltersComponentValue::SetCurrentFilter)
        .Field("autoRise", [](SlotTypeFiltersComponentValue*) { return false; }, nullptr)
        .Field("addButtonIcon", [](SlotTypeFiltersComponentValue*) { return DAVA::TArc::SharedIcon(":/QtIcons/cplus.png"); }, nullptr)
        .Field("addButtonTooltip", [](SlotTypeFiltersComponentValue*) { return "Add type filter"; }, nullptr)
        .Field("addButtonEnabled", [](SlotTypeFiltersComponentValue* v) { return v->filters.size() < DAVA::SlotComponent::MAX_FILTERS_COUNT; }, nullptr)
        .Method("addTypeFilter", &SlotTypeFiltersComponentValue::AddTypeFilter)
        .Field("removeButtonIcon", [](SlotTypeFiltersComponentValue*) { return DAVA::TArc::SharedIcon(":/QtIcons/cminus.png"); }, nullptr)
        .Field("removeButtonTooltip", [](SlotTypeFiltersComponentValue*) { return "Remove selected type filter"; }, nullptr)
        .Field("removeButtonEnabled", [](SlotTypeFiltersComponentValue* v) { return v->currentFilter.empty() == false; }, nullptr)
        .Method("removeTypeFilter", &SlotTypeFiltersComponentValue::RemoveTypeFilter)

        .Field("addFilterPopupText", &SlotTypeFiltersComponentValue::GetPopupText, &SlotTypeFiltersComponentValue::SetPopupText)
        .Field("filterEditPlaceholder", [](SlotTypeFiltersComponentValue*) { return "Type filter name"; }, nullptr)
        .End();
    }
};

class SlotJointComponentValue : public BaseSlotComponentValue
{
public:
    DAVA::Any GetMultipleValue() const override
    {
        return DAVA::Any();
    }

    bool IsValidValueToSet(const DAVA::Any& newValue, const DAVA::Any& currentValue) const override
    {
        return false;
    }

    DAVA::TArc::ControlProxy* CreateEditorWidget(QWidget* parent, const DAVA::Reflection& model, DAVA::TArc::DataWrappersProcessor* wrappersProcessor) override
    {
        using namespace DAVA::TArc;

        ComboBox::Params params(GetAccessor(), GetUI(), GetWindowKey());
        params.fields[ComboBox::Fields::Enumerator] = "itemsList";
        params.fields[ComboBox::Fields::Value] = "currentJoint";
        params.fields[ComboBox::Fields::IsReadOnly] = BaseComponentValue::readOnlyFieldName;
        ComboBox* combo = new ComboBox(params, wrappersProcessor, model, parent);
        return combo;
    }

private:
    static const DAVA::String DetachItemName;
    const DAVA::Map<DAVA::String, DAVA::String>& GetJoints() const
    {
        joints.clear();
        ForEachSlotComponent([&](DAVA::SlotComponent* component, bool isFirst)
                             {
                                 if (isFirst == true)
                                 {
                                     DAVA::SkeletonComponent* skeleton = GetSkeletonComponent(component->GetEntity());
                                     if (skeleton != nullptr)
                                     {
                                         for (DAVA::uint32 i = 0; i < skeleton->GetJointsCount(); ++i)
                                         {
                                             const DAVA::SkeletonComponent::Joint& joint = skeleton->GetJoint(i);
                                             joints.emplace(joint.uid.c_str(), joint.name.c_str());
                                         }
                                     }
                                 }
                                 else
                                 {
                                     DAVA::Map<DAVA::String, DAVA::String> jointIntersection;
                                     DAVA::SkeletonComponent* skeleton = GetSkeletonComponent(component->GetEntity());
                                     if (skeleton != nullptr)
                                     {
                                         for (DAVA::uint32 i = 0; i < skeleton->GetJointsCount(); ++i)
                                         {
                                             const DAVA::SkeletonComponent::Joint& joint = skeleton->GetJoint(i);
                                             joints.emplace(joint.uid.c_str(), joint.name.c_str());
                                         }
                                     }

                                     std::swap(joints, jointIntersection);
                                 }
                                 return true;
                             });

        joints.emplace(DetachItemName, DetachItemName);
        return joints;
    }

    DAVA::Any GetCurrentJoint() const
    {
        DAVA::FastName result(DAVA::TArc::MultipleValuesString);

        ForEachSlotComponent([&](DAVA::SlotComponent* component, bool isFirst)
                             {
                                 if (isFirst == true)
                                 {
                                     result = component->GetJointUID();
                                 }
                                 else
                                 {
                                     DAVA::FastName jointName = component->GetJointUID();
                                     if (jointName != result)
                                     {
                                         result = DAVA::FastName(DAVA::TArc::MultipleValuesString);
                                         return false;
                                     }
                                 }

                                 return true;
                             });

        if (result.IsValid() == false)
        {
            return DAVA::Any(DetachItemName);
        }

        return DAVA::Any(DAVA::String(result.c_str()));
    }

    void SetCurrentJoint(const DAVA::Any& jointName)
    {
        if (jointName.IsEmpty())
        {
            return;
        }

        DAVA::String v = jointName.Cast<DAVA::String>();
        DAVA::Any newValue = DAVA::FastName(v);
        if (v == DetachItemName)
        {
            newValue = DAVA::FastName();
        }

        std::shared_ptr<DAVA::TArc::ModifyExtension> ext = GetModifyInterface();
        DAVA::TArc::ModifyExtension::MultiCommandInterface cmdInterface = ext->GetMultiCommandInterface("Attach to joint", static_cast<DAVA::uint32>(nodes.size()));
        ForEachSlotComponent([&](DAVA::SlotComponent* component, bool isFirst)
                             {
                                 DAVA::Reflection slotReflection = DAVA::Reflection::Create(DAVA::ReflectedObject(component));
                                 DAVA::Reflection jointNameField = slotReflection.GetField(DAVA::SlotComponent::AttchementToJointFieldName);
                                 DVASSERT(jointNameField.IsValid() == true);
                                 DAVA::Reflection::Field f;
                                 f.key = DAVA::SlotComponent::AttchementToJointFieldName;
                                 f.ref = jointNameField;
                                 cmdInterface.ProduceCommand(f, newValue);
                                 ;
                                 return true;
                             });
    }

    bool IsReadOnly() const override
    {
        if (BaseComponentValue::IsReadOnly() == true)
        {
            return true;
        }

        bool result = true;
        ForEachSlotComponent([&](DAVA::SlotComponent* component, bool isFirst)
                             {
                                 DAVA::SkeletonComponent* skeleton = GetSkeletonComponent(component->GetEntity());
                                 if (isFirst == true)
                                 {
                                     result = skeleton == nullptr;
                                 }
                                 else
                                 {
                                     result &= skeleton == nullptr;
                                 }
                                 return result;
                             });
        return result;
    }

    mutable DAVA::Map<DAVA::String, DAVA::String> joints;

    DAVA_VIRTUAL_REFLECTION_IN_PLACE(SlotJointComponentValue, BaseSlotComponentValue)
    {
        DAVA::ReflectionRegistrator<SlotJointComponentValue>::Begin()
        .Field("itemsList", &SlotJointComponentValue::GetJoints, nullptr)
        .Field("currentJoint", &SlotJointComponentValue::GetCurrentJoint, &SlotJointComponentValue::SetCurrentJoint)
        .End();
    }
};

const DAVA::String SlotJointComponentValue::DetachItemName = DAVA::String("< Detached >");

class SlotPreviewComponentValue : public BaseSlotComponentValue
{
public:
    DAVA::Any GetMultipleValue() const override
    {
        return DAVA::Any();
    }

    bool IsValidValueToSet(const DAVA::Any& newValue, const DAVA::Any& currentValue) const override
    {
        return false;
    }

    DAVA::TArc::ControlProxy* CreateEditorWidget(QWidget* parent, const DAVA::Reflection& model, DAVA::TArc::DataWrappersProcessor* wrappersProcessor) override
    {
        using namespace DAVA::TArc;

        ComboBox::Params params(GetAccessor(), GetUI(), GetWindowKey());
        params.fields[ComboBox::Fields::Enumerator] = "itemsList";
        params.fields[ComboBox::Fields::Value] = "currentPreviewItem";
        params.fields[ComboBox::Fields::IsReadOnly] = "previewItemReadOnly";
        ComboBox* combo = new ComboBox(params, wrappersProcessor, model, parent);
        return combo;
    }

private:
    void UpdateValues() const
    {
        DAVA::Any currentConfig;
        DAVA::Set<DAVA::FastName> currentFilters;
        ForEachSlotComponent([&](DAVA::SlotComponent* component, bool isFirst) {
            if (isFirst == true)
            {
                currentConfig = DAVA::Any(component->GetConfigFilePath());
                for (DAVA::uint32 i = 0; i < component->GetTypeFiltersCount(); ++i)
                {
                    currentFilters.insert(component->GetTypeFilter(i));
                }
            }
            else
            {
                if (currentConfig != component->GetConfigFilePath())
                {
                    currentConfig = DAVA::TArc::MultipleValuesString;
                }

                DAVA::Set<DAVA::FastName> filtersIntersection;
                for (DAVA::uint32 i = 0; i < component->GetTypeFiltersCount(); ++i)
                {
                    DAVA::FastName typeFilter = component->GetTypeFilter(i);
                    if (currentFilters.count(typeFilter) > 0)
                    {
                        filtersIntersection.insert(typeFilter);
                    }
                }

                std::swap(currentFilters, filtersIntersection);
            }
            return true;
        });

        if (currentConfig != configPath || currentFilters != filters)
        {
            configPath = currentConfig;
            filters = currentFilters;
            RebuildItemsList();
        }
        else
        {
            DAVA::RefPtr<SceneEditor2> scene = GetAccessor()->GetActiveContext()->GetData<SceneData>()->GetScene();
            if (configPath.CanGet<DAVA::FilePath>() && scene->slotSystem->IsConfigParsed(configPath.Get<DAVA::FilePath>()) == false)
            {
                RebuildItemsList();
            }
        }
    }

    DAVA::String GetCurrentItem() const
    {
        DAVA::FastName item = GetLoadedItemInfo();
        DVASSERT(item.IsValid());
        DAVA::String strItem(item.c_str());
        if (itemsList.find(strItem) == itemsList.end())
        {
            RebuildItemsList();
        }
        return strItem;
    }

    void SetCurrentItem(const DAVA::String& item)
    {
        using namespace DAVA::TArc;

        DAVA::FastName itemName(item);
        DAVA::RefPtr<SceneEditor2> scene = GetAccessor()->GetActiveContext()->GetData<SceneData>()->GetScene();

        std::shared_ptr<ModifyExtension> extension = GetModifyInterface();
        ModifyExtension::MultiCommandInterface cmdInterface = extension->GetMultiCommandInterface("Load preview item to slot", static_cast<DAVA::uint32>(nodes.size()));
        ForEachSlotComponent([&](DAVA::SlotComponent* component, bool) {
            cmdInterface.Exec(std::make_unique<AttachEntityToSlot>(scene.Get(), component, itemName));
            return true;
        });

        RebuildItemsList();
    }

    bool IsPreviewReadOnly() const
    {
        UpdateValues();
        return configPath.CanGet<DAVA::FilePath>() == false;
    }

    DAVA::FastName GetLoadedItemInfo() const
    {
        DAVA::FastName item;
        ForEachSlotComponent([&](DAVA::SlotComponent* component, bool isFirst) {
            DAVA::FastName loadedItem = component->GetLoadedItemName();
            if (isFirst == true)
            {
                item = loadedItem;
            }
            else if (item != loadedItem)
            {
                item = DAVA::FastName(DAVA::TArc::MultipleValuesString);
                return false;
            }

            return true;
        });

        if (item.IsValid() == false)
        {
            return EditorSlotSystem::emptyItemName;
        }

        return item;
    }

    void RebuildItemsList() const
    {
        using namespace DAVA;

        itemsList.clear();
        itemsList.emplace(EditorSlotSystem::emptyItemName.c_str(), EditorSlotSystem::emptyItemName);
        DAVA::FastName item = GetLoadedItemInfo();
        if (configPath.CanGet<DAVA::FilePath>() == true)
        {
            DAVA::FilePath path = configPath.Get<DAVA::FilePath>();
            DAVA::RefPtr<SceneEditor2> scene = GetAccessor()->GetActiveContext()->GetData<SceneData>()->GetScene();

            Vector<DAVA::SlotSystem::ItemsCache::Item> items = scene->slotSystem->GetItems(path);
            for (const DAVA::SlotSystem::ItemsCache::Item& item : items)
            {
                if (filters.empty() == true || filters.count(item.type) > 0)
                {
                    itemsList.emplace(item.itemName.c_str(), item.itemName);
                }
            }
        }

        if (itemsList.find(item.c_str()) == itemsList.end())
        {
            itemsList.emplace(item.c_str(), item);
        }
    }

    const DAVA::Map<DAVA::String, DAVA::FastName>& GetItemsList() const
    {
        UpdateValues();
        return itemsList;
    }

    mutable DAVA::Any configPath;
    mutable DAVA::Set<DAVA::FastName> filters;
    mutable DAVA::Map<DAVA::String, DAVA::FastName> itemsList;

    DAVA_VIRTUAL_REFLECTION_IN_PLACE(SlotPreviewComponentValue, BaseSlotComponentValue)
    {
        DAVA::ReflectionRegistrator<SlotPreviewComponentValue>::Begin()
        .Field("currentPreviewItem", &SlotPreviewComponentValue::GetCurrentItem, &SlotPreviewComponentValue::SetCurrentItem)
        .Field("previewItemReadOnly", &SlotPreviewComponentValue::IsPreviewReadOnly, nullptr)
        .Field("itemsList", &SlotPreviewComponentValue::GetItemsList, nullptr)
        .End();
    }
};

class SlotTemplateComponentValue : public DAVA::TArc::BaseComponentValue
{
public:
    DAVA::Any GetMultipleValue() const override
    {
        return DAVA::Any();
    }

    bool IsValidValueToSet(const DAVA::Any& newValue, const DAVA::Any& currentValue) const override
    {
        if (currentValue.IsEmpty())
        {
            return true;
        }

        return newValue.IsEmpty() == true || newValue != currentValue;
    }

    DAVA::TArc::ControlProxy* CreateEditorWidget(QWidget* parent, const DAVA::Reflection& model, DAVA::TArc::DataWrappersProcessor* wrappersProcessor) override
    {
        DAVA::TArc::ContextAccessor* accessor = GetAccessor();
        SlotTemplatesData* data = accessor->GetGlobalContext()->GetData<SlotTemplatesData>();
        DAVA::Vector<SlotTemplatesData::Template> templates = data->GetTemplates();
        for (const SlotTemplatesData::Template& t : templates)
        {
            enumerator.emplace(t.name.c_str());
        }

        DAVA::TArc::ComboBox::Params params(accessor, GetUI(), GetWindowKey());
        params.fields[DAVA::TArc::ComboBox::Fields::Enumerator] = "enumerator";
        params.fields[DAVA::TArc::ComboBox::Fields::Value] = "value";
        params.fields[DAVA::TArc::ComboBox::Fields::IsReadOnly] = "isEmpty";
        params.fields[DAVA::TArc::ComboBox::Fields::MultipleValueText] = "emptyEnumText";
        return new DAVA::TArc::ComboBox(params, wrappersProcessor, model, parent);
    }

private:
    DAVA::Any GetTemplateName() const
    {
        DAVA::Any fastNameAny = GetValue();
        if (fastNameAny.IsEmpty())
        {
            return fastNameAny;
        }

        return fastNameAny.Cast<DAVA::String>();
    }

    void SetTemplateName(const DAVA::Any& templateName)
    {
        SetValue(templateName);
    }

    DAVA::Set<DAVA::String> enumerator;
    DAVA_VIRTUAL_REFLECTION_IN_PLACE(SlotTemplateComponentValue, DAVA::TArc::BaseComponentValue)
    {
        DAVA::ReflectionRegistrator<SlotTemplateComponentValue>::Begin()
        .Field("enumerator", &SlotTemplateComponentValue::enumerator)
        .Field("value", &SlotTemplateComponentValue::GetTemplateName, &SlotTemplateComponentValue::SetTemplateName)
        .Field("isEmpty", [](SlotTemplateComponentValue* obj) { return obj->enumerator.empty(); }, nullptr)
        .Field("emptyEnumText", [](SlotTemplateComponentValue*) { return "Empty"; }, nullptr)
        .End();
    }
};

class SlotNameComponentValue : public DAVA::TArc::TextComponentValue
{
public:
    bool IsReadOnly() const override
    {
        return DAVA::TArc::TextComponentValue::IsReadOnly() || GetAccessor()->GetGlobalContext()->GetData<SlotSystemSettings>()->autoGenerateSlotNames;
    }
};

class PasteSlotComponentValue : public DAVA::TArc::BaseComponentValue
{
public:
    DAVA::Any GetMultipleValue() const override
    {
        return DAVA::Any();
    }

    bool IsSpannedControl() const override
    {
        return true;
    }

    bool IsValidValueToSet(const DAVA::Any& newValue, const DAVA::Any& currentValue) const override
    {
        return false;
    }

    DAVA::TArc::ControlProxy* CreateEditorWidget(QWidget* parent, const DAVA::Reflection& model, DAVA::TArc::DataWrappersProcessor* wrappersProcessor) override
    {
        using namespace DAVA::TArc;
        ReflectedPushButton::Params params(GetAccessor(), GetUI(), GetWindowKey());
        params.fields[ReflectedPushButton::Fields::Clicked] = "paste";
        params.fields[ReflectedPushButton::Fields::Enabled] = "isEnabled";
        params.fields[ReflectedPushButton::Fields::Text] = "text";

        return new ReflectedPushButton(params, wrappersProcessor, model, parent);
    }

    static bool IsPasteAvailable()
    {
        QClipboard* clipboard = DAVA::PlatformApi::Qt::GetApplication()->clipboard();
        const QMimeData* mimeData = clipboard->mimeData();
        return mimeData->hasFormat("mime:/propertyPanel/slotComponent");
    }

private:
    bool IsEnabled() const
    {
        return nodes.size() == 1;
    }

    void Paste()
    {
        auto showNotification = [&]()
        {
            DAVA::TArc::NotificationParams p;
            p.message.message = "Clipboard doesn't contain slot component. Please copy slot to clipboard first";
            p.message.type = DAVA::Result::RESULT_ERROR;
            p.title = "Slot can't be pasted";
            GetUI()->ShowNotification(DAVA::TArc::mainWindowKey, p);
        };

        QClipboard* clipboard = DAVA::PlatformApi::Qt::GetApplication()->clipboard();
        const QMimeData* mimeData = clipboard->mimeData();
        QByteArray slotArray = mimeData->data("mime:/propertyPanel/slotComponent");
        if (slotArray.isEmpty() == true)
        {
            showNotification();
            return;
        }

        DAVA::RefPtr<DAVA::KeyedArchive> archive(new DAVA::KeyedArchive());
        if (archive->Load(reinterpret_cast<const DAVA::uint8*>(slotArray.data()), slotArray.size()) == false)
        {
            showNotification();
            return;
        }

        DAVA::TArc::DataContext* activeContext = GetAccessor()->GetActiveContext();
        DVASSERT(activeContext != nullptr);
        SceneData* sceneData = activeContext->GetData<SceneData>();
        SceneData::TSceneType scene = sceneData->GetScene();
        DAVA::FilePath scenePath = sceneData->GetScenePath();
        DAVA::SerializationContext ctx;
        ctx.SetVersion(DAVA::VersionInfo::Instance()->GetCurrentVersion().version);
        ctx.SetScene(scene.Get());
        ctx.SetScenePath(DAVA::FilePath());
        ctx.SetRootNodePath(scenePath);

        DAVA::SlotComponent* newComponent = new DAVA::SlotComponent();
        newComponent->Deserialize(archive.Get(), &ctx);

        DAVA::TArc::ModifyExtension::MultiCommandInterface cmd = GetModifyInterface()->GetMultiCommandInterface("Paste slot component", 1);
        DAVA::Entity* entity = nodes.front()->field.ref.GetValueObject().GetPtr<DAVA::Entity>();
        cmd.Exec(std::make_unique<AddComponentCommand>(entity, newComponent));
    }

    DAVA_VIRTUAL_REFLECTION_IN_PLACE(PasteSlotComponentValue, DAVA::TArc::BaseComponentValue)
    {
        DAVA::ReflectionRegistrator<PasteSlotComponentValue>::Begin()
        .Field("isEnabled", &PasteSlotComponentValue::IsEnabled, nullptr)
        .Field("text", []() { return "Paste slot component"; }, nullptr)
        .Method("paste", &PasteSlotComponentValue::Paste)
        .End();
    }
};

void SlotComponentChildCreator::ExposeChildren(const std::shared_ptr<DAVA::TArc::PropertyNode>& parent, DAVA::Vector<std::shared_ptr<DAVA::TArc::PropertyNode>>& children) const
{
    if (parent->propertyType == SlotPasteProperty)
    {
        return;
    }

    if (parent->propertyType == DAVA::TArc::PropertyNode::SelfRoot && PasteSlotComponentValue::IsPasteAvailable() == true)
    {
        DAVA::Reflection::Field f = parent->field;
        std::shared_ptr<DAVA::TArc::PropertyNode> previewNode = allocator->CreatePropertyNode(parent, std::move(f), DAVA::TArc::PropertyNode::InvalidSortKey - 2, SlotPasteProperty);
        children.push_back(previewNode);
    }

    if (parent->propertyType == SlotPreviewProperty ||
        parent->propertyType == SlotTypeFilters ||
        parent->propertyType == SlotJointAttachment)
    {
        return;
    }

    ChildCreatorExtension::ExposeChildren(parent, children);
    const DAVA::ReflectedType* fieldType = DAVA::TArc::GetValueReflectedType(parent->field.ref);
    const DAVA::ReflectedType* slotComponentType = DAVA::ReflectedTypeDB::Get<DAVA::SlotComponent>();
    if (fieldType == slotComponentType)
    {
        {
            auto iter = std::find_if(children.rbegin(), children.rend(), [](const std::shared_ptr<DAVA::TArc::PropertyNode>& node)
                                     {
                                         return node->field.key.Cast<DAVA::FastName>() == DAVA::SlotComponent::AttchementToJointFieldName;
                                     });

            DVASSERT(iter != children.rend());
            std::shared_ptr<DAVA::TArc::PropertyNode> jointAttachmentNode = *iter;
            jointAttachmentNode->propertyType = SlotJointAttachment;
            const DAVA::M::DisplayName* displayNameMeta = jointAttachmentNode->field.ref.GetMeta<DAVA::M::DisplayName>();
            if (displayNameMeta != nullptr)
            {
                jointAttachmentNode->field.key = displayNameMeta->displayName;
            }
            jointAttachmentNode->field.ref = parent->field.ref;
            jointAttachmentNode->cachedValue = parent->cachedValue;
        }

        {
            auto iter = std::find_if(children.rbegin(), children.rend(), [](const std::shared_ptr<DAVA::TArc::PropertyNode>& node)
                                     {
                                         return node->field.key.Cast<DAVA::FastName>() == DAVA::SlotComponent::TemplateFieldName;
                                     });

            DVASSERT(iter != children.rend());
            std::shared_ptr<DAVA::TArc::PropertyNode> templateNameNode = *iter;
            templateNameNode->propertyType = SlotTemplateName;
        }

        {
            if (IsDeveloperMode() == true)
            {
                DAVA::Reflection::Field f;
                f.key = DAVA::FastName("Type Filters");
                f.ref = parent->field.ref;
                std::shared_ptr<DAVA::TArc::PropertyNode> previewNode = allocator->CreatePropertyNode(parent, std::move(f), static_cast<DAVA::int32>(children.size()), SlotTypeFilters);
                children.push_back(previewNode);
            }
        }

        {
            DAVA::Reflection::Field f;
            f.key = DAVA::FastName("Loaded item");
            f.ref = parent->field.ref;
            std::shared_ptr<DAVA::TArc::PropertyNode> previewNode = allocator->CreatePropertyNode(parent, std::move(f), static_cast<DAVA::int32>(children.size()), SlotPreviewProperty);
            children.push_back(previewNode);
        }
    }
}

std::unique_ptr<DAVA::TArc::BaseComponentValue> SlotComponentEditorCreator::GetEditor(const std::shared_ptr<const DAVA::TArc::PropertyNode>& node) const
{
    if (node->propertyType == SlotTypeFilters)
    {
        return std::make_unique<SlotTypeFiltersComponentValue>();
    }

    if (node->propertyType == SlotPreviewProperty)
    {
        return std::make_unique<SlotPreviewComponentValue>();
    }

    if (node->propertyType == SlotJointAttachment)
    {
        return std::make_unique<SlotJointComponentValue>();
    }

    if (node->propertyType == SlotTemplateName)
    {
        return std::make_unique<SlotTemplateComponentValue>();
    }

    if (node->field.key == DAVA::SlotComponent::SlotNameFieldName)
    {
        DAVA::ReflectedObject obj = node->field.ref.GetDirectObject();
        if (obj.GetReflectedType() == DAVA::ReflectedTypeDB::Get<DAVA::SlotComponent>())
        {
            return std::make_unique<SlotNameComponentValue>();
        }
    }

    if (node->propertyType == SlotPasteProperty)
    {
        return std::make_unique<PasteSlotComponentValue>();
    }

    return EditorComponentExtension::GetEditor(node);
}

class GenerateUniqueName : public DAVA::M::CommandProducer
{
public:
    bool IsApplyable(const std::shared_ptr<DAVA::TArc::PropertyNode>& node) const override
    {
        return true;
    }

    Info GetInfo() const override
    {
        Info info;
        info.description = "Unique slot name generated";
        info.tooltip = "Generate unique slot name";
        info.icon = DAVA::TArc::SharedIcon(":/QtIcons/rebuild_name.png");

        return info;
    }

    std::unique_ptr<DAVA::Command> CreateCommand(const std::shared_ptr<DAVA::TArc::PropertyNode>& node, const Params& params) const override
    {
        DAVA::ReflectedObject slotObject = node->field.ref.GetDirectObject();
        DAVA::SlotComponent* component = slotObject.GetPtr<DAVA::SlotComponent>();

        DAVA::FastName name = EditorSlotSystem::GenerateUniqueSlotName(component);

        return std::make_unique<SetFieldValueCommand>(node->field, name);
    }
};

class InvalidateSlotConfigCache : public DAVA::M::CommandProducer
{
public:
    bool IsApplyable(const std::shared_ptr<DAVA::TArc::PropertyNode>& node) const override
    {
        return true;
    }

    Info GetInfo() const override
    {
        Info info;
        info.description = "Slot config reloading";
        info.tooltip = "Reload config from disk";
        info.icon = DAVA::TArc::SharedIcon(":/QtIcons/reloadtextures.png");

        return info;
    }

    std::unique_ptr<DAVA::Command> CreateCommand(const std::shared_ptr<DAVA::TArc::PropertyNode>& node, const Params& params) const override
    {
        DAVA::TArc::DataContext* ctx = params.accessor->GetActiveContext();
        DVASSERT(ctx != nullptr);

        DAVA::Any value = node->field.ref.GetValue();
        if (value.CanCast<DAVA::FilePath>())
        {
            DAVA::FilePath path = value.Cast<DAVA::FilePath>();
            SceneData* data = ctx->GetData<SceneData>();
            DVASSERT(data != nullptr);
            data->GetScene()->slotSystem->InvalidateConfig(path);
        }
        return nullptr;
    }
};

class CopySlotProducer : public DAVA::M::CommandProducer
{
public:
    bool IsApplyable(const std::shared_ptr<DAVA::TArc::PropertyNode>& node) const override
    {
        return true;
    }

    bool OnlyForSingleSelection() const override
    {
        return true;
    }

    Info GetInfo() const override
    {
        Info info;
        info.description = "Copy slot to clipboard";
        info.tooltip = "Copy slot to clipboard";
        info.icon = DAVA::TArc::SharedIcon(":/QtIcons/clone_inplace.png");

        return info;
    }

    std::unique_ptr<DAVA::Command> CreateCommand(const std::shared_ptr<DAVA::TArc::PropertyNode>& node, const Params& params) const override
    {
        DVASSERT(params.accessor->GetActiveContext() != nullptr);
        SceneData* data = params.accessor->GetActiveContext()->GetData<SceneData>();
        SceneData::TSceneType scene = data->GetScene();
        if (scene->IsLoaded() == false)
        {
            DAVA::TArc::NotificationParams p;
            p.message.message = "You should save scene at least once first";
            p.message.type = DAVA::Result::RESULT_ERROR;
            p.title = "Slot can't be copied";
            params.ui->ShowNotification(DAVA::TArc::mainWindowKey, p);
            return nullptr;
        }

        DAVA::FilePath scenePath = data->GetScenePath();
        DAVA::SerializationContext ctx;
        ctx.SetVersion(DAVA::VersionInfo::Instance()->GetCurrentVersion().version);
        ctx.SetScene(scene.Get());
        ctx.SetScenePath(DAVA::FilePath());
        ctx.SetRootNodePath(scenePath);

        DAVA::SlotComponent* slotComponent = node->field.ref.GetValueObject().GetPtr<DAVA::SlotComponent>();
        DAVA::RefPtr<DAVA::KeyedArchive> archive(new DAVA::KeyedArchive());
        slotComponent->Serialize(archive.Get(), &ctx);

        DAVA::uint32 archiveSize = archive->Save(nullptr, 0);
        QByteArray byteArray;
        byteArray.resize(archiveSize);
        archive->Save(reinterpret_cast<DAVA::uint8*>(byteArray.data()), archiveSize);

        QMimeData* mimeData = new QMimeData();
        mimeData->setData("mime:/propertyPanel/slotComponent", byteArray);
        QClipboard* clipboard = DAVA::PlatformApi::Qt::GetApplication()->clipboard();
        clipboard->setMimeData(mimeData);

        return nullptr;
    }
};

std::shared_ptr<DAVA::M::CommandProducer> CreateCopySlotProducer()
{
    return std::make_shared<CopySlotProducer>();
}

DAVA::M::CommandProducerHolder CreateSlotNameCommandProvider()
{
    DAVA::M::CommandProducerHolder holder;
    holder.AddCommandProducer(std::make_shared<GenerateUniqueName>());

    return holder;
}

DAVA::M::CommandProducerHolder CreateSlotConfigCommandProvider()
{
    DAVA::M::CommandProducerHolder holder;
    holder.AddCommandProducer(std::make_shared<InvalidateSlotConfigCache>());

    return holder;
}
}

#include "Classes/PropertyPanel/KeyedArchiveExtensions.h"
#include "Classes/PropertyPanel/PropertyPanelCommon.h"
#include "Classes/PropertyPanel/Private/KeyedArchiveEditors.h"

#include <REPlatform/Commands/KeyedArchiveCommand.h>
#include <REPlatform/DataNodes/ProjectManagerData.h>
#include <REPlatform/Deprecated/EditorConfig.h>

#include <TArc/Controls/PropertyPanel/BaseComponentValue.h>
#include <TArc/Controls/PropertyPanel/PropertyModelExtensions.h>
#include <TArc/Controls/PropertyPanel/PropertyPanelMeta.h>
#include <TArc/Utils/ReflectionHelpers.h>
#include <TArc/Utils/Utils.h>

#include <FileSystem/KeyedArchive.h>

namespace KeyedArchiveExtensionDetail
{
class RemoveKeyedArchiveItem : public DAVA::M::CommandProducer
{
public:
    bool IsApplyable(const std::shared_ptr<DAVA::PropertyNode>& node) const
    {
        return true;
    }

    Info GetInfo() const
    {
        Info info;
        info.icon = DAVA::SharedIcon(":/QtIcons/keyminus.png");
        info.description = "Remove keyed archive member";
        info.tooltip = QStringLiteral("Remove keyed archive member");

        return info;
    }

    std::unique_ptr<DAVA::Command> CreateCommand(const std::shared_ptr<DAVA::PropertyNode>& node, const Params& params) const
    {
        std::shared_ptr<DAVA::PropertyNode> parent = node->parent.lock();
        DVASSERT(parent != nullptr);

        DAVA::ReflectedObject refObject = parent->field.ref.GetValueObject();
        const DAVA::ReflectedType* refType = refObject.GetReflectedType();

        DAVA::KeyedArchive* archive = nullptr;
        if (refType == DAVA::ReflectedTypeDB::Get<DAVA::KeyedArchive>())
        {
            archive = refObject.GetPtr<DAVA::KeyedArchive>();
        }
        else if (refType == DAVA::ReflectedTypeDB::Get<DAVA::KeyedArchive*>())
        {
            archive = *refObject.GetPtr<DAVA::KeyedArchive*>();
        }

        DVASSERT(archive != nullptr);
        if (archive != nullptr)
        {
            DAVA::String key = node->field.key.Cast<DAVA::String>();
            if (archive->Count(key) > 0)
            {
                return std::make_unique<DAVA::KeyeadArchiveRemValueCommand>(archive, key);
            }
        }

        return nullptr;
    }
};
}

KeyedArchiveChildCreator::KeyedArchiveChildCreator()
{
    DAVA::M::CommandProducerHolder holder;
    holder.AddCommandProducer(std::make_shared<KeyedArchiveExtensionDetail::RemoveKeyedArchiveItem>());
    elementsMeta.reset(new DAVA::ReflectedMeta(std::move(holder)));
}

void KeyedArchiveChildCreator::ExposeChildren(const std::shared_ptr<DAVA::PropertyNode>& parent, DAVA::Vector<std::shared_ptr<DAVA::PropertyNode>>& children) const
{
    using namespace DAVA;
    using namespace DAVA;

    if (parent->cachedValue.GetType() == Type::Instance<KeyedArchive*>())
    {
        Vector<Reflection::Field> fields = parent->field.ref.GetFields();
        std::sort(fields.begin(), fields.end(), [](const Reflection::Field& node1, const Reflection::Field& node2)
                  {
                      return node1.key.Cast<String>() < node2.key.Cast<String>();
                  });

        for (Reflection::Field& f : fields)
        {
            f.ref = Reflection::Create(f.ref, elementsMeta.get());
            std::shared_ptr<PropertyNode> node = allocator->CreatePropertyNode(parent, std::move(f), static_cast<int32>(children.size()), PropertyNode::RealProperty);
            node->idPostfix = FastName(node->cachedValue.GetType()->GetName());
            children.push_back(node);
        }
    }
    else
    {
        ChildCreatorExtension::ExposeChildren(parent, children);
    }
}

KeyedArchiveEditorCreator::KeyedArchiveEditorCreator(DAVA::ContextAccessor* accessor_)
    : accessor(accessor_)
{
}

std::unique_ptr<DAVA::BaseComponentValue> KeyedArchiveEditorCreator::GetEditor(const std::shared_ptr<const DAVA::PropertyNode>& node) const
{
    using namespace DAVA;
    using namespace DAVA;

    if (node->propertyType == PropertyNode::RealProperty && node->cachedValue.GetType() == Type::Instance<KeyedArchive*>())
    {
        ProjectManagerData* data = accessor->GetGlobalContext()->GetData<ProjectManagerData>();
        DVASSERT(data);

        const EditorConfig* editorConfig = data->GetEditorConfig();
        const DAVA::Vector<DAVA::String>& presets = editorConfig->GetProjectPropertyNames();
        Vector<DAVA::VariantType> defaultValues;
        defaultValues.reserve(presets.size());
        for (const DAVA::String& name : presets)
        {
            defaultValues.push_back(*editorConfig->GetPropertyDefaultValue(name));
        }
        return std::make_unique<PropertyPanel::KeyedArchiveEditor>(presets, defaultValues);
    }

    std::shared_ptr<DAVA::PropertyNode> parent = node->parent.lock();
    if (parent != nullptr && parent->propertyType == PropertyNode::RealProperty && parent->cachedValue.GetType() == Type::Instance<KeyedArchive*>())
    {
        String key = node->field.key.Cast<String>();

        ProjectManagerData* data = accessor->GetGlobalContext()->GetData<ProjectManagerData>();
        DVASSERT(data);

        const EditorConfig* editorConfig = data->GetEditorConfig();
        int32 presetType = editorConfig->GetPropertyValueType(key);
        if (presetType != VariantType::TYPE_NONE)
        {
            int32 valueVariantType = VariantType::TYPE_NONE;
            const Type* valueType = node->cachedValue.GetType();
            if (valueType == Type::Instance<int32>())
            {
                Vector<Any> allowedValues;
                const Vector<String>& presetValues = editorConfig->GetComboPropertyValues(key);
                if (presetValues.empty() == false)
                {
                    std::for_each(presetValues.begin(), presetValues.end(), [&allowedValues](const String& v)
                                  {
                                      allowedValues.push_back(v);
                                  });
                }

                const Vector<Color>& presetColors = editorConfig->GetColorPropertyValues(key);
                if (presetColors.empty() == false)
                {
                    std::for_each(presetColors.begin(), presetColors.end(), [&allowedValues](const Color& v)
                                  {
                                      allowedValues.push_back(v);
                                  });
                }

                if (allowedValues.empty() == false)
                {
                    return std::make_unique<PropertyPanel::KeyedArchiveComboPresetEditor>(allowedValues);
                }
            }
        }
    }

    return EditorComponentExtension::GetEditor(node);
}

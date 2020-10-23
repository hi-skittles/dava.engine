#include "Classes/PropertyPanel/PropertyModelExt.h"
#include "Classes/PropertyPanel/PropertyPanelCommon.h"
#include "Classes/Qt/DockParticleEditor/TimeLineWidget.h"

#include <REPlatform/Commands/AddComponentCommand.h>
#include <REPlatform/Commands/KeyedArchiveCommand.h>
#include <REPlatform/Commands/SetFieldValueCommand.h>
#include <REPlatform/DataNodes/SceneData.h>

#include <TArc/Controls/ComboBox.h>
#include <TArc/Controls/CommonStrings.h>
#include <TArc/Controls/PropertyPanel/PropertyModelExtensions.h>
#include <TArc/Controls/PropertyPanel/PropertyPanelMeta.h>
#include <TArc/Controls/Widget.h>
#include <TArc/Utils/QtConnections.h>
#include <TArc/Utils/ReflectionHelpers.h>
#include <TArc/Utils/Utils.h>
#include <TArc/Utils/ReflectedPairsVector.h>


#include <Base/Any.h>
#include <Base/FastName.h>
#include <Base/StaticSingleton.h>
#include <Base/TypeInheritance.h>
#include <Engine/PlatformApiQt.h>
#include <Entity/ComponentManager.h>
#include <Entity/Component.h>
#include <FileSystem/KeyedArchive.h>
#include <Functional/Function.h>
#include <Reflection/ReflectedMeta.h>
#include <Reflection/ReflectedTypeDB.h>
#include <Reflection/Reflection.h>
#include <Scene3D/Entity.h>

#include <QHBoxLayout>
#include <QPalette>
#include <QToolButton>

namespace PropertyModelExtDetails
{
using namespace DAVA;

struct ComponentCreator : public StaticSingleton<ComponentCreator>
{
    const ReflectedType* componentType = nullptr;
};

const char* chooseComponentTypeString = "Choose component type for Add";

struct TypeInitializer : public StaticSingleton<ComponentCreator>
{
    using TypePair = std::pair<String, const ReflectedType*>;
    struct TypePairLess
    {
        bool operator()(const TypePair& p1, const TypePair& p2) const
        {
            if (p1.second == nullptr && p2.second == nullptr)
            {
                return false;
            }

            if (p2.second == nullptr)
            {
                return false;
            }

            if (p1.second == nullptr)
            {
                return true;
            }

            return p1.first < p2.first;
        }
    };
    TypeInitializer()
    {
        AnyCast<TypePair, String>::Register([](const Any& v) -> String
                                            {
                                                return v.Get<TypePair>().first;
                                            });

        types.emplace(String(chooseComponentTypeString), nullptr);

        InitDerivedTypes(Type::Instance<DAVA::Component>());
    }

    void InitDerivedTypes(const Type* type)
    {
        const TypeInheritance* inheritance = type->GetInheritance();
        Vector<TypeInheritance::Info> derivedTypes = inheritance->GetDerivedTypes();
        for (const TypeInheritance::Info& derived : derivedTypes)
        {
            const ReflectedType* refType = ReflectedTypeDB::GetByType(derived.type);
            if (refType == nullptr)
            {
                continue;
            }

            const std::unique_ptr<ReflectedMeta>& meta = refType->GetStructure()->meta;
            if (meta != nullptr && (nullptr != meta->GetMeta<M::CantBeCreatedManualyComponent>()))
            {
                continue;
            }

            if (refType->GetCtor(derived.type->Pointer()) != nullptr)
            {
                types.emplace(refType->GetPermanentName(), refType);
            }

            InitDerivedTypes(derived.type);
        }
    }

    Set<TypePair, TypePairLess> types;
};

class ComponentCreatorComponentValue : public BaseComponentValue
{
public:
    ComponentCreatorComponentValue() = default;

    bool IsSpannedControl() const override
    {
        return true;
    }

protected:
    Any GetMultipleValue() const override
    {
        return DAVA::Any();
    }

    bool IsValidValueToSet(const Any& newValue, const Any& currentValue) const override
    {
        return false;
    }

    ControlProxy* CreateEditorWidget(QWidget* parent, const Reflection& model, DataWrappersProcessor* wrappersProcessor) override
    {
        Widget* w = new Widget(parent);
        QHBoxLayout* layout = new QHBoxLayout();
        layout->setMargin(0);
        layout->setSpacing(2);
        w->SetLayout(layout);

        ComponentCreatorComponentValue* nonConstThis = const_cast<ComponentCreatorComponentValue*>(this);
        nonConstThis->toolButton = new QToolButton(w->ToWidgetCast());
        nonConstThis->toolButton->setIcon(SharedIcon(":/QtIcons/addcomponent.png"));
        nonConstThis->toolButton->setIconSize(toolButtonIconSize);
        nonConstThis->toolButton->setToolTip(QStringLiteral("Add component"));
        nonConstThis->toolButton->setAutoRaise(true);
        layout->addWidget(nonConstThis->toolButton.data());
        nonConstThis->connections.AddConnection(nonConstThis->toolButton.data(), &QToolButton::clicked, MakeFunction(nonConstThis, &ComponentCreatorComponentValue::AddComponent));

        ComboBox::Params params(GetAccessor(), GetUI(), GetWindowKey());
        params.fields[ComboBox::Fields::Enumerator] = "types";
        params.fields[ComboBox::Fields::Value] = "currentType";
        w->AddControl(new ComboBox(params, wrappersProcessor, model, w->ToWidgetCast()));

        return w;
    }

private:
    TypeInitializer::TypePair GetType() const
    {
        const ReflectedType* type = ComponentCreator::Instance()->componentType;
        if (type == nullptr)
        {
            toolButton->setEnabled(false);
            return TypeInitializer::TypePair(String(chooseComponentTypeString), nullptr);
        }

        toolButton->setEnabled(true);
        return TypeInitializer::TypePair(type->GetPermanentName(), type);
    }

    void SetType(const TypeInitializer::TypePair& type)
    {
        toolButton->setEnabled(type.second != nullptr);
        ComponentCreator::Instance()->componentType = type.second;
    }

    const Set<TypeInitializer::TypePair, TypeInitializer::TypePairLess>& GetTypes() const
    {
        static TypeInitializer t;
        return t.types;
    }

    void AddComponent()
    {
        ComponentCreator* componentCreator = ComponentCreator::Instance();
        const ReflectedType* componentType = componentCreator->componentType;
        DVASSERT(componentType != nullptr);

        String description = Format("Add component: %s", componentType->GetPermanentName().c_str());
        ModifyExtension::MultiCommandInterface cmdInterface = GetModifyInterface()->GetMultiCommandInterface(description, static_cast<uint32>(nodes.size()));
        for (std::shared_ptr<PropertyNode>& node : nodes)
        {
            Entity* entity = node->field.ref.GetValueObject().GetPtr<Entity>();
            Any newComponent = componentType->CreateObject(ReflectedType::CreatePolicy::ByPointer);
            Component* component = newComponent.Cast<Component*>();

            cmdInterface.Exec(std::make_unique<DAVA::AddComponentCommand>(entity, component));
        }

        componentCreator->componentType = nullptr;
    }

    DAVA_VIRTUAL_REFLECTION_IN_PLACE(ComponentCreatorComponentValue, DAVA::BaseComponentValue)
    {
        DAVA::ReflectionRegistrator<ComponentCreatorComponentValue>::Begin()
        .Field("currentType", &ComponentCreatorComponentValue::GetType, &ComponentCreatorComponentValue::SetType)
        .Field("types", &ComponentCreatorComponentValue::GetTypes, nullptr)
        .End();
    }

    QPointer<QToolButton> toolButton;
    QtConnections connections;
};

class ParticlePropertyLineComponentValue : public DAVA::BaseComponentValue
{
protected:
    DAVA::Any GetMultipleValue() const override
    {
        return DAVA::Any();
    }
    bool IsValidValueToSet(const DAVA::Any& newValue, const DAVA::Any& currentValue) const override
    {
        DVASSERT(currentValue.IsEmpty() == false);
        return true;
    }
    DAVA::ControlProxy* CreateEditorWidget(QWidget* parent, const DAVA::Reflection& model, DAVA::DataWrappersProcessor* wrappersProcessor) override
    {
        using namespace DAVA;
        Widget* widget = new Widget(parent);
        QHBoxLayout* layout = new QHBoxLayout();
        widget->SetLayout(layout);
        timeLineWidget = new TimeLineWidget(widget->ToWidgetCast());
        timeLineWidget->setMinimumHeight(800);

        connections.AddConnection(timeLineWidget, &TimeLineWidget::ValueChanged, MakeFunction(this, &ParticlePropertyLineComponentValue::OnWidgetDataChanged));

        layout->addWidget(timeLineWidget);
        UpdateValue();
        wrapper.SetListener(nullptr);
        wrapper = GetDataProcessor()->CreateWrapper([this](const DataContext*) {
            return Reflection::Create(ReflectedObject(this));
        },
                                                    nullptr);
        wrapper.SetListener(&dummyListener);

        widgetInited = false;
        return widget;
    }
    bool IsSpannedControl() const override
    {
        return true;
    }

private:
    void OnWidgetDataChanged()
    {
        DAVA::PropLineWrapper<DAVA::float32> prop;
        timeLineWidget->GetValue(0, prop.GetPropsPtr());
        SetValue(prop.GetPropLine());
    }

    bool UpdateValueHack() const
    {
        const_cast<ParticlePropertyLineComponentValue*>(this)->UpdateValue();
        return true;
    }

    void UpdateValue()
    {
        using namespace DAVA;
        using PropKey = PropertyLine<float32>::PropertyKey;
        auto isEqualFn = [](const Vector<PropKey>& keyCollection1, const Vector<PropKey>& keyCollection2)
        {
            if (keyCollection1.size() != keyCollection2.size())
            {
                return false;
            }

            for (size_t i = 0; i < keyCollection1.size(); ++i)
            {
                const PropKey& key1 = keyCollection1[i];
                const PropKey& key2 = keyCollection2[i];

                if (key1.t != key2.t || key1.value != key2.value)
                {
                    return false;
                }
            }

            return true;
        };
        DAVA::Any val = GetValue();
        if (val.IsEmpty() == false)
        {
            RefPtr<PropertyLine<float32>> accVal = val.Cast<RefPtr<PropertyLine<float32>>>();

            if (widgetInited == false || prevValues != accVal)
            {
                widgetInited = true;
                prevValues = accVal;
                timeLineWidget->Init(0, 1, false);
                timeLineWidget->AddLine(0, PropLineWrapper<float32>(PropertyLineHelper::GetValueLine(accVal)).GetProps(), Qt::red, "Stripe edge size over life");
            }
        }
        else
        {
            timeLineWidget->setEnabled(false);
        }
    }

    class DummnyListener : public DAVA::DataListener
    {
    public:
        void OnDataChanged(const DataWrapper& wrapper, const Vector<Any>& fields) override
        {
        }
    };

    bool widgetInited = false;
    RefPtr<PropertyLine<float32>> prevValues;
    QPointer<TimeLineWidget> timeLineWidget;
    DAVA::QtConnections connections;
    DAVA::DataWrapper wrapper;
    DummnyListener dummyListener;

    DAVA_VIRTUAL_REFLECTION_IN_PLACE(ParticlePropertyLineComponentValue, DAVA::BaseComponentValue)
    {
        DAVA::ReflectionRegistrator<ParticlePropertyLineComponentValue>::Begin()
        .Field("updateValue", &ParticlePropertyLineComponentValue::UpdateValueHack, nullptr)
        .End();
    }
};
}

namespace DAVA
{
template <>
struct AnyCompare<PropertyModelExtDetails::TypeInitializer::TypePair>
{
    static bool IsEqual(const Any& v1, const Any& v2)
    {
        using T = PropertyModelExtDetails::TypeInitializer::TypePair;
        return v1.Get<T>().second == v2.Get<T>().second;
    }
};
} // namespace DAVA

REModifyPropertyExtension::REModifyPropertyExtension(DAVA::ContextAccessor* accessor_)
    : accessor(accessor_)
{
}

void REModifyPropertyExtension::ProduceCommand(const DAVA::Reflection::Field& field, const DAVA::Any& newValue)
{
    GetScene()->Exec(std::make_unique<DAVA::SetFieldValueCommand>(field, newValue));
}

void REModifyPropertyExtension::ProduceCommand(const std::shared_ptr<DAVA::PropertyNode>& node, const DAVA::Any& newValue)
{
    std::shared_ptr<DAVA::PropertyNode> parent = node->parent.lock();
    DVASSERT(parent != nullptr);

    if (parent->cachedValue.CanCast<DAVA::KeyedArchive*>())
    {
        DAVA::String key = node->field.key.Cast<DAVA::String>();
        DAVA::KeyedArchive* archive = parent->cachedValue.Cast<DAVA::KeyedArchive*>();
        DVASSERT(archive != nullptr);
        if (archive != nullptr)
        {
            DVASSERT(archive->Count(key) > 0);
            DAVA::VariantType* currentValue = archive->GetVariant(key);
            DVASSERT(currentValue != nullptr);

            DAVA::VariantType value = DAVA::PrepareValueForKeyedArchive(newValue, currentValue->GetType());
            DVASSERT(value.GetType() != DAVA::VariantType::TYPE_NONE);
            GetScene()->Exec(std::make_unique<DAVA::KeyeadArchiveSetValueCommand>(archive, node->field.key.Cast<DAVA::String>(), value));
        }
    }
    else
    {
        ProduceCommand(node->field, newValue);
    }
}

void REModifyPropertyExtension::Exec(std::unique_ptr<DAVA::Command>&& command)
{
    GetScene()->Exec(std::move(command));
}

void REModifyPropertyExtension::EndBatch()
{
    GetScene()->EndBatch();
}

void REModifyPropertyExtension::BeginBatch(const DAVA::String& text, DAVA::uint32 commandCount)
{
    GetScene()->BeginBatch(text, commandCount);
}

DAVA::SceneEditor2* REModifyPropertyExtension::GetScene() const
{
    using namespace DAVA;
    DataContext* ctx = accessor->GetActiveContext();
    DVASSERT(ctx != nullptr);

    SceneData* data = ctx->GetData<SceneData>();
    DVASSERT(data != nullptr);

    return data->GetScene().Get();
}

void EntityChildCreator::ExposeChildren(const std::shared_ptr<DAVA::PropertyNode>& parent, DAVA::Vector<std::shared_ptr<DAVA::PropertyNode>>& children) const
{
    using namespace DAVA;

    if (parent->propertyType == PropertyPanel::AddComponentProperty)
    {
        return;
    }

    if (parent->propertyType == PropertyNode::SelfRoot &&
        parent->cachedValue.GetType() == DAVA::Type::Instance<DAVA::Entity*>())
    {
        DAVA::Reflection::Field f(Any("Entity"), Reflection(parent->field.ref), nullptr);
        std::shared_ptr<PropertyNode> entityNode = allocator->CreatePropertyNode(parent, std::move(f), -1, PropertyNode::GroupProperty);
        children.push_back(entityNode);

        {
            Entity* entity = parent->field.ref.GetValueObject().GetPtr<Entity>();
            ComponentManager* cm = GetEngineContext()->componentManager;

            for (const Type* type : cm->GetRegisteredSceneComponents())
            {
                uint32 countOftype = entity->GetComponentCount(type);
                for (uint32 componentIndex = 0; componentIndex < countOftype; ++componentIndex)
                {
                    Component* component = entity->GetComponent(type, componentIndex);
                    Reflection ref = Reflection::Create(ReflectedObject(component));
                    String permanentName = GetValueReflectedType(ref)->GetPermanentName();

                    DAVA::Reflection::Field f(permanentName, Reflection(ref), nullptr);
                    if (CanBeExposed(f))
                    {
                        std::shared_ptr<PropertyNode> node = allocator->CreatePropertyNode(parent, std::move(f), cm->GetSortedComponentId(type), PropertyNode::RealProperty);
                        node->idPostfix = FastName(Format("%u", componentIndex));
                        children.push_back(node);
                    }
                }
            }

            Reflection::Field addComponentField;
            addComponentField.key = "Add Component";
            addComponentField.ref = parent->field.ref;
            std::shared_ptr<PropertyNode> addComponentNode = allocator->CreatePropertyNode(parent, std::move(addComponentField), DAVA::PropertyNode::InvalidSortKey - 1, PropertyPanel::AddComponentProperty);
            children.push_back(addComponentNode);
        }
    }
    else if (parent->propertyType == PropertyNode::GroupProperty &&
             parent->cachedValue.GetType() == DAVA::Type::Instance<DAVA::Entity*>())
    {
        DAVA::ForEachField(parent->field.ref, [&](Reflection::Field&& field)
                           {
                               if (field.ref.GetValueType() != DAVA::Type::Instance<DAVA::Vector<DAVA::Component*>>() && CanBeExposed(field))
                               {
                                   children.push_back(allocator->CreatePropertyNode(parent, std::move(field), static_cast<int32>(children.size()), PropertyNode::RealProperty));
                               }
                           });
    }
    else
    {
        ChildCreatorExtension::ExposeChildren(parent, children);
    }
}

std::unique_ptr<DAVA::BaseComponentValue> EntityEditorCreator::GetEditor(const std::shared_ptr<const DAVA::PropertyNode>& node) const
{
    if (node->propertyType == PropertyPanel::AddComponentProperty)
    {
        std::unique_ptr<DAVA::BaseComponentValue> editor = std::make_unique<PropertyModelExtDetails::ComponentCreatorComponentValue>();
        DAVA::BaseComponentValue::Style style;
        style.fontBold = true;
        style.fontItalic = true;
        style.fontColor = QPalette::ButtonText;
        style.bgColor = QPalette::AlternateBase;
        editor->SetStyle(style);
        return std::unique_ptr<DAVA::BaseComponentValue>(std::move(editor));
    }

    const DAVA::Type* valueType = node->cachedValue.GetType();
    static const DAVA::Type* componentType = DAVA::Type::Instance<DAVA::Component*>();
    static const DAVA::Type* entityType = DAVA::Type::Instance<DAVA::Entity*>();

    if ((node->propertyType == DAVA::PropertyNode::GroupProperty && valueType == entityType)
        || (DAVA::TypeInheritance::CanCast(node->cachedValue.GetType(), DAVA::Type::Instance<DAVA::Component*>()) == true))
    {
        std::unique_ptr<DAVA::BaseComponentValue> editor = EditorComponentExtension::GetEditor(node);
        DAVA::BaseComponentValue::Style style;
        style.fontBold = true;
        style.fontColor = QPalette::ButtonText;
        style.bgColor = QPalette::AlternateBase;
        editor->SetStyle(style);
        return std::unique_ptr<DAVA::BaseComponentValue>(std::move(editor));
    }

    return EditorComponentExtension::GetEditor(node);
}

std::unique_ptr<DAVA::BaseComponentValue> ParticleForceCreator::GetEditor(const std::shared_ptr<const DAVA::PropertyNode>& node) const
{
    using namespace DAVA;
    using namespace PropertyModelExtDetails;
    const DAVA::Type* valueType = node->cachedValue.GetType();
    const Type* propertyLineType = Type::Instance<RefPtr<PropertyLine<float32>>>();

    if (node->propertyType == DAVA::PropertyNode::RealProperty && valueType == propertyLineType)
    {
        return std::make_unique<ParticlePropertyLineComponentValue>();
    }

    return EditorComponentExtension::GetEditor(node);
}

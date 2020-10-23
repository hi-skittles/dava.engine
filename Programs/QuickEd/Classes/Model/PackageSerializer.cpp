#include "PackageSerializer.h"

#include "PackageHierarchy/PackageNode.h"
#include "PackageHierarchy/ImportedPackagesNode.h"
#include "PackageHierarchy/PackageControlsNode.h"
#include "PackageHierarchy/ControlNode.h"
#include "PackageHierarchy/StyleSheetsNode.h"
#include "PackageHierarchy/StyleSheetNode.h"

#include "ControlProperties/RootProperty.h"
#include "ControlProperties/ClassProperty.h"
#include "ControlProperties/ComponentPropertiesSection.h"
#include "ControlProperties/ControlPropertiesSection.h"
#include "ControlProperties/CustomClassProperty.h"
#include "ControlProperties/FontValueProperty.h"
#include "ControlProperties/IntrospectionProperty.h"
#include "ControlProperties/LocalizedTextValueProperty.h"
#include "ControlProperties/NameProperty.h"
#include "ControlProperties/PrototypeNameProperty.h"
#include "ControlProperties/StyleSheetRootProperty.h"
#include "ControlProperties/StyleSheetSelectorProperty.h"
#include "ControlProperties/StyleSheetProperty.h"

#include "Utils/QtDavaConvertion.h"

#include <UI/Components/UIComponentUtils.h>
#include <Reflection/ReflectedTypeDB.h>
#include <UI/UIPackage.h>
#include <UI/UIControl.h>
#include <UI/DataBinding/UIDataBindingComponent.h>

#include <Utils/StringFormat.h>

using namespace DAVA;

PackageSerializer::PackageSerializer()
{
}

PackageSerializer::~PackageSerializer()
{
}

void PackageSerializer::SerializePackage(PackageNode* package)
{
    for (int32 i = 0; i < package->GetImportedPackagesNode()->GetCount(); i++)
    {
        importedPackages.push_back(package->GetImportedPackagesNode()->GetImportedPackage(i));
    }

    for (int32 i = 0; i < package->GetStyleSheets()->GetCount(); i++)
    {
        styles.push_back(package->GetStyleSheets()->Get(i));
    }

    for (int32 i = 0; i < package->GetPackageControlsNode()->GetCount(); i++)
    {
        controls.push_back(package->GetPackageControlsNode()->Get(i));
    }

    for (int32 i = 0; i < package->GetPrototypes()->GetCount(); i++)
    {
        prototypes.push_back(package->GetPrototypes()->Get(i));
    }

    package->Accept(this);
    importedPackages.clear();
    controls.clear();
    styles.clear();

    DVASSERT(dataBindings.empty());
}

void PackageSerializer::SerializePackageNodes(PackageNode* package, const DAVA::Vector<ControlNode*>& serializationControls, const DAVA::Vector<StyleSheetNode*>& serializationStyles)
{
    for (ControlNode* control : serializationControls)
    {
        if (control->CanCopy())
        {
            controls.push_back(control);
            CollectPackages(importedPackages, control);
        }
    }

    for (StyleSheetNode* style : serializationStyles)
    {
        if (style->CanCopy())
            styles.push_back(style);
    }

    package->Accept(this);

    importedPackages.clear();
    controls.clear();
    styles.clear();
}

bool PackageSerializer::HasErrors() const
{
    return results.HasErrors();
}

const DAVA::ResultList& PackageSerializer::GetResults() const
{
    return results;
}

void PackageSerializer::VisitPackage(PackageNode* node)
{
    BeginMap("Header");
    PutValue("version", Format("%d", UIPackage::CURRENT_VERSION), true);
    EndMap();

    if (!importedPackages.empty())
    {
        BeginArray("ImportedPackages");
        for (const PackageNode* package : importedPackages)
        {
            if (package->HasErrors())
            {
                results.AddResult(Result(Result::RESULT_ERROR, Format("Package '%s' has errors.", package->GetPath().GetStringValue().c_str())));
            }
            PutValue(package->GetPath().GetFrameworkPath(), true);
        }
        EndArray();
    }

    if (!styles.empty())
    {
        BeginArray("StyleSheets");
        for (StyleSheetNode* style : styles)
            style->Accept(this);
        EndMap();
    }

    if (!prototypes.empty())
    {
        BeginArray("Prototypes");
        for (ControlNode* prototype : prototypes)
            prototype->Accept(this);
        EndArray();
    }

    if (!controls.empty())
    {
        BeginArray("Controls");
        for (ControlNode* control : controls)
            control->Accept(this);
        EndArray();
    }

    if (node->HasCustomData())
    {
        PutCustomData(node);
    }
}

void PackageSerializer::VisitImportedPackages(ImportedPackagesNode* node)
{
    // do nothing
}

void PackageSerializer::VisitControls(PackageControlsNode* node)
{
    // do nothing
}

void PackageSerializer::VisitControl(ControlNode* node)
{
    if (node->HasErrors())
    {
        results.AddResult(Result(Result::RESULT_ERROR, Format("Control '%s' has errors.", node->GetName().c_str())));
    }
    BeginMap();

    node->GetRootProperty()->Accept(this);

    if (node->GetCount() > 0)
    {
        bool shouldProcessChildren = true;
        Vector<ControlNode*> prototypeChildrenWithChanges;

        if (node->GetCreationType() == ControlNode::CREATED_FROM_PROTOTYPE)
        {
            CollectPrototypeChildrenWithChanges(node, prototypeChildrenWithChanges);
            shouldProcessChildren = !prototypeChildrenWithChanges.empty() || HasNonPrototypeChildren(node);
        }

        if (shouldProcessChildren)
        {
            BeginArray("children");

            for (const auto& child : prototypeChildrenWithChanges)
                child->Accept(this);

            for (int32 i = 0; i < node->GetCount(); i++)
            {
                if (node->Get(i)->GetCreationType() != ControlNode::CREATED_FROM_PROTOTYPE_CHILD)
                    node->Get(i)->Accept(this);
            }

            EndArray();
        }
    }

    EndMap();
}

void PackageSerializer::VisitStyleSheets(StyleSheetsNode* node)
{
    // do nothing
}

void PackageSerializer::VisitStyleSheet(StyleSheetNode* node)
{
    BeginMap();
    node->GetRootProperty()->Accept(this);
    EndMap();
}

void PackageSerializer::AcceptChildren(PackageBaseNode* node)
{
    for (int32 i = 0; i < node->GetCount(); i++)
        node->Get(i)->Accept(this);
}

void PackageSerializer::CollectPackages(Vector<PackageNode*>& packages, const ControlNode* node) const
{
    if (node->GetCreationType() == ControlNode::CREATED_FROM_PROTOTYPE)
    {
        ControlNode* prototype = node->GetPrototype();
        if (prototype && std::find(packages.begin(), packages.end(), prototype->GetPackage()) == packages.end())
        {
            packages.push_back(prototype->GetPackage());
        }
    }

    if (node->GetPackage())
    {
        for (int32 index = 0; index < node->GetPackage()->GetImportedPackagesNode()->GetCount(); index++)
        {
            PackageNode* package = node->GetPackage()->GetImportedPackagesNode()->GetImportedPackage(index);
            if (IsControlNodeDependsOnStylesFromPackage(node, package))
            {
                packages.push_back(package);
            }
        }
    }

    for (int32 index = 0; index < node->GetCount(); index++)
        CollectPackages(packages, node->Get(index));
}

bool PackageSerializer::IsControlNodeDependsOnStylesFromPackage(const ControlNode* node, const PackageNode* package) const
{
    StyleSheetsNode* styles = package->GetStyleSheets();
    for (StyleSheetNode* ssNode : *styles)
    {
        StyleSheetRootProperty* root = ssNode->GetRootProperty();
        StyleSheetSelectorsSection* selectorsSection = root->GetSelectors();
        for (StyleSheetSelectorProperty* selectorProperty : *selectorsSection)
        {
            const UIStyleSheetSelectorChain& chain = selectorProperty->GetSelectorChain();
            for (const UIStyleSheetSelector& selector : chain)
            {
                for (const FastName& cl : selector.classes)
                {
                    if (node->GetControl()->HasClass(cl))
                    {
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

bool PackageSerializer::IsControlInSerializationList(const ControlNode* control) const
{
    return std::find(controls.begin(), controls.end(), control) != controls.end();
}

void PackageSerializer::CollectPrototypeChildrenWithChanges(const ControlNode* node, Vector<ControlNode*>& out) const
{
    for (int32 i = 0; i < node->GetCount(); i++)
    {
        ControlNode* child = node->Get(i);
        if (child->GetCreationType() == ControlNode::CREATED_FROM_PROTOTYPE_CHILD)
        {
            if (HasNonPrototypeChildren(child) || child->GetRootProperty()->HasChanges())
                out.push_back(child);

            CollectPrototypeChildrenWithChanges(child, out);
        }
    }
}

bool PackageSerializer::HasNonPrototypeChildren(const ControlNode* node) const
{
    for (int32 i = 0; i < node->GetCount(); i++)
    {
        if (node->Get(i)->GetCreationType() != ControlNode::CREATED_FROM_PROTOTYPE_CHILD)
            return true;
    }
    return false;
}

// ---

void PackageSerializer::VisitRootProperty(RootProperty* property)
{
    DVASSERT(dataBindings.empty());

    property->GetPrototypeProperty()->Accept(this);
    property->GetClassProperty()->Accept(this);
    property->GetCustomClassProperty()->Accept(this);
    property->GetNameProperty()->Accept(this);

    for (int32 i = 0; i < property->GetControlPropertiesSectionsCount(); i++)
        property->GetControlPropertiesSection(i)->Accept(this);

    bool hasChanges = false;

    for (const ComponentPropertiesSection* section : property->GetComponents())
    {
        if (section->HasChanges())
        {
            hasChanges = true;
            break;
        }
    }

    if (hasChanges)
    {
        BeginMap("components");

        for (const auto section : property->GetComponents())
            section->Accept(this);

        EndMap();
    }

    if (!dataBindings.empty())
    {
        BeginArray("bindings");
        for (const DataBindingInfo& info : dataBindings)
        {
            BeginFlowArray();
            PutValue(info.fieldName, true);
            PutValue(info.expression, true);
            PutValue(info.mode, true);
            EndArray();
        }
        dataBindings.clear();

        EndArray();
    }
}

void PackageSerializer::VisitControlSection(ControlPropertiesSection* property)
{
    AcceptChildren(property);
}

void PackageSerializer::VisitComponentSection(ComponentPropertiesSection* property)
{
    if (property->HasChanges() && property->GetComponentType() != Type::Instance<UIDataBindingComponent>())
    {
        String name = property->GetComponentName();
        if (UIComponentUtils::IsMultiple(property->GetComponentType()))
            name += Format("%d", property->GetComponentIndex());

        BeginMap(name);
        AcceptChildren(property);
        EndMap();
    }
}

void PackageSerializer::VisitNameProperty(NameProperty* property)
{
    switch (property->GetControlNode()->GetCreationType())
    {
    case ControlNode::CREATED_FROM_PROTOTYPE:
    case ControlNode::CREATED_FROM_CLASS:
        PutValue("name", property->GetControlNode()->GetName(), true);
        break;

    case ControlNode::CREATED_FROM_PROTOTYPE_CHILD:
        PutValue("path", property->GetControlNode()->GetPathToPrototypeChild(), true);
        break;

    default:
        DVASSERT(false);
    }
}

void PackageSerializer::VisitPrototypeNameProperty(PrototypeNameProperty* property)
{
    if (property->GetControl()->GetCreationType() == ControlNode::CREATED_FROM_PROTOTYPE)
    {
        ControlNode* prototype = property->GetControl()->GetPrototype();

        String name = "";
        PackageNode* prototypePackage = prototype->GetPackage();
        if (std::find(importedPackages.begin(), importedPackages.end(), prototypePackage) != importedPackages.end())
        {
            name = prototypePackage->GetName() + "/";
        }
        name += prototype->GetName();

        PutValue("prototype", name, true);
    }
}

void PackageSerializer::VisitClassProperty(ClassProperty* property)
{
    if (property->GetControlNode()->GetCreationType() == ControlNode::CREATED_FROM_CLASS)
    {
        PutValue("class", property->GetClassName(), true);
    }
}

void PackageSerializer::VisitCustomClassProperty(CustomClassProperty* property)
{
    if (property->IsOverriddenLocally())
    {
        PutValue("customClass", property->GetCustomClassName(), true);
    }
}

void PackageSerializer::VisitIntrospectionProperty(IntrospectionProperty* property)
{
    if (property->IsOverriddenLocally())
    {
        if (property->IsBound())
        {
            DataBindingInfo info;
            info.fieldName = property->GetFullFieldName();
            info.expression = property->GetBindingExpression();
            info.mode = GlobalEnumMap<UIDataBindingComponent::UpdateMode>::Instance()->ToString(property->GetBindingUpdateMode());
            dataBindings.push_back(info);
        }
        else
        {
            PutValueProperty(property->GetName(), property);
        }
    }
}

void PackageSerializer::VisitStyleSheetRoot(StyleSheetRootProperty* property)
{
    PutValue("selector", property->GetSelectorsAsString(), true);

    BeginMap("properties", false);
    if (property->GetPropertiesSection()->GetCount() > 0)
    {
        AcceptChildren(property->GetPropertiesSection());
    }
    EndMap();
}

void PackageSerializer::VisitStyleSheetSelectorProperty(StyleSheetSelectorProperty* property)
{
    // do nothing
}

void PackageSerializer::VisitStyleSheetProperty(StyleSheetProperty* property)
{
    if (property->HasTransition())
    {
        BeginMap(property->GetName());
        PutValueProperty("value", property);
        PutValue("transitionTime", Format("%f", property->GetTransitionTime()), false);

        const EnumMap* enumMap = GlobalEnumMap<Interpolation::FuncType>::Instance();
        PutValue("transitionFunction", enumMap->ToString(property->GetTransitionFunction()), true);
        EndMap();
    }
    else
    {
        PutValueProperty(property->GetName(), property);
    }
}

void PackageSerializer::AcceptChildren(AbstractProperty* property)
{
    for (uint32 i = 0; i < property->GetCount(); i++)
    {
        property->GetProperty(i)->Accept(this);
    }
}

void PackageSerializer::PutValueProperty(const DAVA::String& name, ValueProperty* property)
{
    Any value = property->GetSerializationValue();

    if (property->GetType() == AbstractProperty::TYPE_FLAGS)
    {
        Vector<String> values;
        const EnumMap* enumMap = property->GetEnumMap();
        int val = value.Cast<int32>();
        int p = 1;
        while (val > 0)
        {
            if ((val & 0x01) != 0)
                values.push_back(enumMap->ToString(p));
            val >>= 1;
            p <<= 1;
        }
        PutValue(name, values);
    }
    else if (property->GetType() == AbstractProperty::TYPE_ENUM)
    {
        PutValue(name, property->GetEnumMap()->ToString(value.Cast<int32>()), true);
    }
    else if (value.CanGet<Vector2>())
    {
        BeginArray(name, true);
        const Vector2& vector = value.Get<Vector2>();
        PutValue(AnyToString(vector.x), false);
        PutValue(AnyToString(vector.y), false);
        EndArray();
    }
    else if (value.CanGet<Vector3>())
    {
        BeginArray(name, true);
        const Vector3& vector = value.Get<Vector3>();
        PutValue(AnyToString(vector.x), false);
        PutValue(AnyToString(vector.y), false);
        PutValue(AnyToString(vector.z), false);
        EndArray();
    }
    else if (value.CanGet<Vector4>())
    {
        BeginArray(name, true);
        const Vector4& vector = value.Get<Vector4>();
        PutValue(AnyToString(vector.x), false);
        PutValue(AnyToString(vector.y), false);
        PutValue(AnyToString(vector.z), false);
        PutValue(AnyToString(vector.w), false);
        EndArray();
    }
    else if (value.CanGet<Color>())
    {
        BeginArray(name, true);
        const Color& color = value.Get<Color>();
        PutValue(AnyToString(color.r), false);
        PutValue(AnyToString(color.g), false);
        PutValue(AnyToString(color.b), false);
        PutValue(AnyToString(color.a), false);
        EndArray();
    }
    else
    {
        PutValue(name, AnyToString(value), value.CanGet<String>() || value.CanGet<FilePath>() || value.CanGet<FastName>());
    }
}

void PackageSerializer::PutCustomData(const PackageNode* node)
{
    BeginMap("CustomData");
    PutGuides(node);
    EndMap();
}

void PackageSerializer::PutGuides(const PackageNode* node)
{
    BeginMap("Guides");

    Vector<PackageControlsNode*> packageControlsContainers = {
        node->GetPackageControlsNode(), node->GetPrototypes()
    };
    Set<String> names;
    for (PackageControlsNode* packageControlsContainer : packageControlsContainers)
    {
        for (int i = 0, count = packageControlsContainer->GetCount(); i < count; ++i)
        {
            ControlNode* rootControl = packageControlsContainer->Get(i);
            names.insert(rootControl->GetName());
        }
    }
    for (const String& name : names)
    {
        const PackageNode::Guides& mapItemValue = node->GetGuides(name);
        if (mapItemValue[Vector2::AXIS_X].empty() == false || mapItemValue[Vector2::AXIS_Y].empty() == false)
        {
            BeginMap(name);
            {
                if (mapItemValue[Vector2::AXIS_X].empty() == false)
                {
                    BeginArray("Vertical");
                    PutGuidesList(mapItemValue[Vector2::AXIS_X]);
                    EndArray();
                }
                if (mapItemValue[Vector2::AXIS_Y].empty() == false)
                {
                    BeginArray("Horizontal");
                    PutGuidesList(mapItemValue[Vector2::AXIS_Y]);
                    EndArray();
                }
            }
            EndMap();
        }
    }
    EndMap();
}

void PackageSerializer::PutGuidesList(const PackageNode::AxisGuides& values)
{
    for (float32 value : values)
    {
        PutValue(AnyToString(value), false);
    }
}

#include "Classes/Model/QuickEdPackageBuilder.h"

#include "Classes/Model/ControlProperties/ComponentPropertiesSection.h"
#include "Classes/Model/ControlProperties/ControlPropertiesSection.h"
#include "Classes/Model/ControlProperties/CustomClassProperty.h"
#include "Classes/Model/ControlProperties/NameProperty.h"
#include "Classes/Model/ControlProperties/RootProperty.h"
#include "Classes/Model/ControlProperties/ValueProperty.h"
#include "Classes/Model/PackageHierarchy/ControlNode.h"
#include "Classes/Model/PackageHierarchy/ImportedPackagesNode.h"
#include "Classes/Model/PackageHierarchy/PackageControlsNode.h"
#include "Classes/Model/PackageHierarchy/PackageNode.h"
#include "Classes/Model/PackageHierarchy/StyleSheetNode.h"
#include "Classes/Model/PackageHierarchy/StyleSheetsNode.h"

#include <Base/ObjectFactory.h>
#include <Entity/ComponentManager.h>
#include <FileSystem/YamlNode.h>
#include <Reflection/ReflectedTypeDB.h>
#include <UI/Styles/UIStyleSheet.h>
#include <UI/Styles/UIStyleSheetYamlLoader.h>
#include <UI/DataBinding/UIDataBindingComponent.h>
#include <UI/UIControl.h>
#include <UI/UIControlPackageContext.h>
#include <UI/UIPackage.h>
#include <UI/Components/UIComponentUtils.h>
#include <Utils/Utils.h>

using namespace DAVA;

namespace QuickEdPackageBuilderDetails
{
const String EXCEPTION_CLASS_UI_TEXT_FIELD = "UITextField";
const String EXCEPTION_CLASS_UI_LIST = "UIList";

struct GuidesOrientation
{
    String type;
    PackageNode::AxisGuides* values;
};
}

QuickEdPackageBuilder::QuickEdPackageBuilder(const EngineContext* engineContext_)
    : currentObject(nullptr)
    , currentSection(nullptr)
    , engineContext(engineContext_)
{
}

QuickEdPackageBuilder::~QuickEdPackageBuilder()
{
    for (PackageNode* importedPackage : importedPackages)
        importedPackage->Release();
    importedPackages.clear();

    for (ControlNode* control : rootControls)
        control->Release();
    rootControls.clear();

    for (ControlNode* prototype : prototypes)
        prototype->Release();
    prototypes.clear();

    for (StyleSheetNode* styleSheet : styleSheets)
        styleSheet->Release();
    styleSheets.clear();
}

void QuickEdPackageBuilder::BeginPackage(const FilePath& aPackagePath, int32 version)
{
    DVASSERT(packagePath.IsEmpty());
    packagePath = aPackagePath;
}

void QuickEdPackageBuilder::EndPackage()
{
}

bool QuickEdPackageBuilder::ProcessImportedPackage(const String& packagePathStr, AbstractUIPackageLoader* loader)
{
    FilePath packagePath(packagePathStr);
    for (PackageNode* package : importedPackages)
    {
        if (package->GetPath().GetFrameworkPath() == packagePath.GetFrameworkPath())
            return true;
    }

    if (std::find(declinedPackages.begin(), declinedPackages.end(), packagePath) != declinedPackages.end())
    {
        Result r(Result::RESULT_ERROR, Format("Can't import package '%s' (to prevent cyclic imports)", packagePathStr.c_str()));
        results.AddResult(r);
        return false;
    }

    QuickEdPackageBuilder builder(engineContext);
    builder.declinedPackages.insert(builder.declinedPackages.end(), declinedPackages.begin(), declinedPackages.end());
    builder.declinedPackages.push_back(packagePath);

    if (loader->LoadPackage(packagePath, &builder))
    {
        RefPtr<PackageNode> importedPackage = builder.BuildPackage();
        importedPackages.push_back(SafeRetain(importedPackage.Get()));
        return true;
    }

    Result r(Result::RESULT_ERROR, Format("Can't import package '%s'", packagePathStr.c_str()));
    results.AddResult(r);
    PackageNode* fakeNode = new PackageNode(packagePathStr);
    fakeNode->AddResult(r);
    importedPackages.push_back(fakeNode);

    return false;
}

void QuickEdPackageBuilder::ProcessStyleSheet(const DAVA::Vector<DAVA::UIStyleSheetSelectorChain>& selectorChains, const DAVA::Vector<DAVA::UIStyleSheetProperty>& properties)
{
    UIStyleSheetSourceInfo sourceInfo(packagePath);

    StyleSheetNode* node = new StyleSheetNode(sourceInfo, selectorChains, properties);
    styleSheets.push_back(node);
}

const ReflectedType* QuickEdPackageBuilder::BeginControlWithClass(const FastName& controlName, const String& className)
{
    RefPtr<UIControl> control(ObjectFactory::Instance()->New<UIControl>(className));
    ControlNode* node = ControlNode::CreateFromControl(control.Get());
    if (control.Valid())
    {
        if (className != QuickEdPackageBuilderDetails::EXCEPTION_CLASS_UI_TEXT_FIELD && className != QuickEdPackageBuilderDetails::EXCEPTION_CLASS_UI_LIST) //TODO: fix internal staticText for Win\Mac
        {
            control->RemoveAllControls();
        }

        if (controlName.IsValid())
        {
            node->GetRootProperty()->GetNameProperty()->SetValue(controlName);
        }
    }

    controlsStack.push_back(ControlDescr(node, true));

    return (control != nullptr) ? ReflectedTypeDB::GetByPointer(control.Get()) : nullptr;
}

const ReflectedType* QuickEdPackageBuilder::BeginControlWithCustomClass(const FastName& controlName, const String& customClassName, const String& className)
{
    RefPtr<UIControl> control;

    if (ObjectFactory::Instance()->IsTypeRegistered(customClassName))
    {
        control.Set(ObjectFactory::Instance()->New<UIControl>(customClassName));
    }
    else
    {
        control.Set(ObjectFactory::Instance()->New<UIControl>(className));
    }

    ControlNode* node = ControlNode::CreateFromControl(control.Get());
    if (control.Valid())
    {
        if (className != QuickEdPackageBuilderDetails::EXCEPTION_CLASS_UI_TEXT_FIELD && className != QuickEdPackageBuilderDetails::EXCEPTION_CLASS_UI_LIST) //TODO: fix internal staticText for Win\Mac
        {
            control->RemoveAllControls();
        }

        if (controlName.IsValid())
        {
            node->GetRootProperty()->GetNameProperty()->SetValue(controlName);
        }
    }

    node->GetRootProperty()->GetCustomClassProperty()->SetForceOverride(true);
    node->GetRootProperty()->GetCustomClassProperty()->SetValue(customClassName);

    controlsStack.push_back(ControlDescr(node, true));

    return (control != nullptr) ? ReflectedTypeDB::GetByPointer(control.Get()) : nullptr;
}

const ReflectedType* QuickEdPackageBuilder::BeginControlWithPrototype(const FastName& controlName, const String& packageName, const FastName& prototypeFastName, const String* customClassName, AbstractUIPackageLoader* loader)
{
    ControlNode* prototypeNode = nullptr;
    String prototypeName(prototypeFastName.c_str());
    if (packageName.empty())
    {
        prototypeNode = FindPrototype(prototypeFastName);
        if (prototypeNode == nullptr)
        {
            if (loader->LoadControlByName(prototypeFastName, this))
                prototypeNode = FindPrototype(prototypeFastName);
        }
    }
    else
    {
        for (PackageNode* importedPackage : importedPackages)
        {
            if (importedPackage->GetName() == packageName)
            {
                prototypeNode = importedPackage->GetPrototypes()->FindChildByName(prototypeName);
                if (prototypeNode == nullptr)
                {
                    prototypeNode = importedPackage->GetPackageControlsNode()->FindChildByName(prototypeName);
                }
                break;
            }
        }
    }

    ControlNode* node = nullptr;

    if (prototypeNode)
    {
        node = ControlNode::CreateFromPrototype(prototypeNode);
    }
    else
    {
        RefPtr<UIControl> fakeControl(new UIControl());
        node = ControlNode::CreateFromControl(fakeControl.Get());

        String errorMsg = Format("Can't find prototype '%s' from package '%s'",
                                 prototypeFastName.c_str(), packageName.c_str());
        Result r(Result::RESULT_ERROR, errorMsg);
        results.AddResult(r);
        node->AddResult(r);
    }

    if (customClassName)
    {
        node->GetRootProperty()->GetCustomClassProperty()->SetForceOverride(true);
        node->GetRootProperty()->GetCustomClassProperty()->SetValue(*customClassName);
    }

    if (controlName.IsValid())
    {
        node->GetRootProperty()->GetNameProperty()->SetValue(controlName);
    }
    controlsStack.push_back(ControlDescr(node, true));

    return ReflectedTypeDB::GetByPointer(node->GetControl());
}

const ReflectedType* QuickEdPackageBuilder::BeginControlWithPath(const String& pathName)
{
    ControlNode* control = nullptr;
    if (!controlsStack.empty())
    {
        control = controlsStack.back().node;
        Vector<String> controlNames;
        Split(pathName, "/", controlNames, false, true);
        for (const String& controlName : controlNames)
        {
            ControlNode* child = control->FindChildByName(controlName);
            if (child == nullptr)
            {
                RefPtr<UIControl> fakeControl(new UIControl());
                RefPtr<ControlNode> newChild(ControlNode::CreateFromControl(fakeControl.Get()));
                newChild->GetRootProperty()->GetNameProperty()->SetValue(controlName);

                results.AddResult(Result(Result::RESULT_ERROR, Format("Access to removed control by path '%s'", pathName.c_str())));
                newChild->AddResult(Result(Result::RESULT_ERROR, "Control was removed in prototype"));
                control->Add(newChild.Get());
                child = newChild.Get();
            }

            control = child;
        }
    }

    controlsStack.push_back(ControlDescr(SafeRetain(control), false));
    return ReflectedTypeDB::GetByPointer(control->GetControl());
}

const ReflectedType* QuickEdPackageBuilder::BeginUnknownControl(const FastName& controlName, const YamlNode* node)
{
    DVASSERT(false);
    return nullptr;
}

void QuickEdPackageBuilder::EndControl(eControlPlace controlPlace)
{
    ControlNode* lastControl = SafeRetain(controlsStack.back().node);

    // the following code handles cases when component was created by control himself (UIParticles creates UIUpdateComponent for example)
    ComponentManager* cm = engineContext->componentManager;
    auto& components = cm->GetRegisteredUIComponents();
    for (auto& c : components)
    {
        const ComponentPropertiesSection* section = lastControl->GetRootProperty()->FindComponentPropertiesSection(c, 0);

        if (section == nullptr && lastControl->GetControl()->GetComponentCount(c) > 0 &&
            !UIComponentUtils::IsHidden(c))
        {
            BeginComponentPropertiesSection(c, 0);
            EndComponentPropertiesSection();
        }
    }

    bool addToParent = controlsStack.back().addToParent;
    controlsStack.pop_back();

    if (addToParent)
    {
        switch (controlPlace)
        {
        case TO_CONTROLS:
            rootControls.push_back(SafeRetain(lastControl));
            break;

        case TO_PROTOTYPES:
            prototypes.push_back(SafeRetain(lastControl));
            break;

        case TO_PREVIOUS_CONTROL:
            DVASSERT(!controlsStack.empty());
            controlsStack.back().node->Add(lastControl);
            break;

        default:
            DVASSERT(false);
            break;
        }
    }

    lastControl->GetControl()->LoadFromYamlNodeCompleted();

    SafeRelease(lastControl);
}

void QuickEdPackageBuilder::BeginControlPropertiesSection(const String& name)
{
    currentSection = controlsStack.back().node->GetRootProperty()->GetControlPropertiesSection(name);
    DVASSERT(currentSection != nullptr);
    currentObject = controlsStack.back().node->GetControl();
    DVASSERT(currentObject != nullptr);
}

void QuickEdPackageBuilder::EndControlPropertiesSection()
{
    currentSection = nullptr;
    currentObject = nullptr;
}

const ReflectedType* QuickEdPackageBuilder::BeginComponentPropertiesSection(const Type* componentType, DAVA::uint32 componentIndex)
{
    ControlNode* node = controlsStack.back().node;
    ComponentPropertiesSection* section;
    section = node->GetRootProperty()->FindComponentPropertiesSection(componentType, componentIndex);
    if (section == nullptr)
        section = node->GetRootProperty()->AddComponentPropertiesSection(componentType);
    currentObject = section->GetComponent();
    currentSection = section;
    return ReflectedTypeDB::GetByPointer(section->GetComponent());
}

void QuickEdPackageBuilder::EndComponentPropertiesSection()
{
    currentSection = nullptr;
    currentObject = nullptr;
}

void QuickEdPackageBuilder::ProcessProperty(const ReflectedStructure::Field& field, const DAVA::Any& value)
{
    if (currentObject && currentSection)
    {
        ValueProperty* property = currentSection->FindChildPropertyByName(field.name.c_str());
        if (property && !value.IsEmpty())
        {
            if (property->GetStylePropertyIndex() != -1)
                controlsStack.back().node->GetControl()->SetPropertyLocalFlag(property->GetStylePropertyIndex(), true);

            property->SetForceOverride(true);
            property->SetValue(value);
        }
    }
    else
    {
        DVASSERT(false);
    }
}

void QuickEdPackageBuilder::ProcessDataBinding(const DAVA::String& fieldName, const DAVA::String& expression, DAVA::int32 bindingMode)
{
    Vector<String> pathParts;
    Split(fieldName, ".", pathParts);

    ControlNode* node = controlsStack.back().node;
    RootProperty* rootProperty = node->GetRootProperty();

    AbstractProperty* property = nullptr;
    if (pathParts.size() > 1)
    {
        for (ComponentPropertiesSection* section : rootProperty->GetComponents())
        {
            if (section->GetName() == pathParts[0])
            {
                property = section->FindPropertyByName(pathParts[1]);
                break;
            }
        }
    }
    else
    {
        for (int32 i = 0; i < rootProperty->GetControlPropertiesSectionsCount(); i++)
        {
            property = rootProperty->GetControlPropertiesSection(i)->FindPropertyByName(pathParts[0]);
            if (property)
            {
                break;
            }
        }
    }

    if (property)
    {
        property->SetBindingExpression(expression, bindingMode);
    }
}

void QuickEdPackageBuilder::ProcessCustomData(const YamlNode* customDataNode)
{
    DVASSERT(customDataNode != nullptr);
    DVASSERT(customDataNode->GetType() == YamlNode::TYPE_MAP);

    const YamlNode* guidesNode = customDataNode->Get("Guides");
    if (guidesNode != nullptr)
    {
        ProcessGuides(guidesNode);
    }
}

void QuickEdPackageBuilder::ProcessGuides(const DAVA::YamlNode* guidesNode)
{
    const auto& controlsMap = guidesNode->AsMap();
    for (const auto& controlsMapItem : controlsMap)
    {
        const String& controlName = controlsMapItem.first;
        YamlNode* allGuidesNode = controlsMapItem.second.Get();
        PackageNode::Guides& guides = allGuides[controlName];

        Vector<QuickEdPackageBuilderDetails::GuidesOrientation> orientations = { { "Vertical", &guides[Vector2::AXIS_X] }, { "Horizontal", &guides[Vector2::AXIS_Y] } };
        for (QuickEdPackageBuilderDetails::GuidesOrientation& orientation : orientations)
        {
            const YamlNode* guideValuesNode = allGuidesNode->Get(orientation.type);
            if (guideValuesNode != nullptr)
            {
                const auto& valuesNodes = guideValuesNode->AsVector();
                PackageNode::AxisGuides* values = orientation.values;
                if (values->empty() == false)
                {
                    results.AddResult(Result::RESULT_WARNING, Format("Guides for control %s already exists! They will be overwritten", controlName.c_str()));
                    values->clear();
                }
                std::transform(valuesNodes.begin(),
                               valuesNodes.end(),
                               std::back_inserter(*values),
                               [](const RefPtr<YamlNode>& node) {
                                   return node->AsFloat();
                               });
            }
        }
    }
}

RefPtr<PackageNode> QuickEdPackageBuilder::BuildPackage() const
{
    DVASSERT(!packagePath.IsEmpty());
    RefPtr<PackageNode> package(new PackageNode(packagePath));

    Vector<PackageNode*> declinedPackages;
    for (PackageNode* importedPackage : importedPackages)
    {
        if (package->GetImportedPackagesNode()->CanInsertImportedPackage(importedPackage))
        {
            package->GetImportedPackagesNode()->Add(importedPackage);
        }
        else
        {
            declinedPackages.push_back(importedPackage);
        }
    }

    for (StyleSheetNode* styleSheet : styleSheets)
    {
        package->GetStyleSheets()->Add(styleSheet);
    }

    for (ControlNode* control : prototypes)
    {
        bool canInsert = true;
        for (PackageNode* declinedPackage : declinedPackages)
        {
            if (control->IsDependsOnPackage(declinedPackage))
            {
                canInsert = false;
                break;
            }
        }

        if (canInsert)
        {
            package->GetPrototypes()->Add(control);
        }
    }

    for (ControlNode* control : rootControls)
    {
        bool canInsert = true;
        for (PackageNode* declinedPackage : declinedPackages)
        {
            if (control->IsDependsOnPackage(declinedPackage))
                canInsert = false;
        }

        if (canInsert)
        {
            package->GetPackageControlsNode()->Add(control);
        }
    }

    for (const auto& mapItem : allGuides)
    {
        package->SetGuides(mapItem.first, mapItem.second);
    }

    package->RefreshPackageStylesAndLayout();

    DVASSERT(declinedPackages.empty());

    return package;
}

const Vector<ControlNode*>& QuickEdPackageBuilder::GetRootControls() const
{
    return rootControls;
}

const Vector<PackageNode*>& QuickEdPackageBuilder::GetImportedPackages() const
{
    return importedPackages;
}

const Vector<StyleSheetNode*>& QuickEdPackageBuilder::GetStyles() const
{
    return styleSheets;
}

void QuickEdPackageBuilder::AddImportedPackage(PackageNode* node)
{
    importedPackages.push_back(SafeRetain(node));
}

const DAVA::ResultList& QuickEdPackageBuilder::GetResults() const
{
    return results;
}

ControlNode* QuickEdPackageBuilder::FindPrototype(const DAVA::FastName& name) const
{
    for (ControlNode* control : prototypes)
    {
        if (control->GetName() == name.c_str())
            return control;
    }

    for (ControlNode* control : rootControls)
    {
        if (control->GetName() == name.c_str())
        {
            return control;
        }
    }
    return nullptr;
}

////////////////////////////////////////////////////////////////////////////////
// ControlDescr
////////////////////////////////////////////////////////////////////////////////
QuickEdPackageBuilder::ControlDescr::ControlDescr()
    : node(nullptr)
    , addToParent(false)
{
}

QuickEdPackageBuilder::ControlDescr::ControlDescr(ControlNode* node, bool addToParent)
    : node(node)
    , addToParent(addToParent)
{
}

QuickEdPackageBuilder::ControlDescr::ControlDescr(const ControlDescr& descr)
{
    node = DAVA::SafeRetain(descr.node);
    addToParent = descr.addToParent;
}

QuickEdPackageBuilder::ControlDescr::~ControlDescr()
{
    DAVA::SafeRelease(node);
}

QuickEdPackageBuilder::ControlDescr& QuickEdPackageBuilder::ControlDescr::operator=(const ControlDescr& descr)
{
    DAVA::SafeRetain(descr.node);
    DAVA::SafeRelease(node);

    node = descr.node;
    addToParent = descr.addToParent;
    return *this;
}

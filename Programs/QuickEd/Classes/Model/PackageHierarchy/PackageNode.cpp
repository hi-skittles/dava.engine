#include "PackageNode.h"

#include "PackageVisitor.h"
#include "PackageControlsNode.h"
#include "ImportedPackagesNode.h"
#include "StyleSheetsNode.h"
#include "PackageListener.h"
#include "Model/ControlProperties/RootProperty.h"
#include "Model/ControlProperties/StyleSheetRootProperty.h"
#include "Model/ControlProperties/StyleSheetSelectorProperty.h"
#include "Model/ControlProperties/ControlPropertiesSection.h"
#include "Model/ControlProperties/ComponentPropertiesSection.h"

#include "Engine/Engine.h"
#include "Engine/EngineContext.h"
#include "UI/Layouts/UILayoutSystem.h"
#include "UI/Styles/UIStyleSheetSystem.h"
#include "UI/UIControl.h"
#include "UI/UIControlSystem.h"
#include "UI/UIControlPackageContext.h"
#include "UI/Styles/UIStyleSheet.h"

using namespace DAVA;

PackageNode::PackageNode(const FilePath& aPath)
    : PackageBaseNode(nullptr)
    , path(aPath)
{
    name = path.GetBasename();
    importedPackagesNode = new ImportedPackagesNode(this);
    packageControlsNode = new PackageControlsNode(this, "Controls");
    prototypes = new PackageControlsNode(this, "Prototypes");
    styleSheets = new StyleSheetsNode(this);
    packageContext = new UIControlPackageContext();
}

PackageNode::~PackageNode()
{
    importedPackagesNode->SetParent(nullptr);
    SafeRelease(importedPackagesNode);

    packageControlsNode->SetParent(nullptr);
    SafeRelease(packageControlsNode);

    prototypes->SetParent(nullptr);
    SafeRelease(prototypes);

    styleSheets->SetParent(nullptr);
    SafeRelease(styleSheets);

    SafeRelease(packageContext);
}

int PackageNode::GetCount() const
{
    if (HasErrors())
    {
        return 0;
    }
    return SECTION_COUNT;
}

PackageBaseNode* PackageNode::Get(int index) const
{
    switch (index)
    {
    case SECTION_IMPORTED_PACKAGES:
        return importedPackagesNode;

    case SECTION_STYLES:
        return styleSheets;

    case SECTION_PROTOTYPES:
        return prototypes;

    case SECTION_CONTROLS:
        return packageControlsNode;
    }
    DVASSERT(false);
    return nullptr;
}

void PackageNode::Accept(PackageVisitor* visitor)
{
    visitor->VisitPackage(this);
}

String PackageNode::GetName() const
{
    return name;
}

PackageNode* PackageNode::GetPackage()
{
    return this;
}

void PackageNode::SetPath(const DAVA::FilePath& path_)
{
    path = path_;
}

const FilePath& PackageNode::GetPath() const
{
    return path;
}

UIControlPackageContext* PackageNode::GetContext() const
{
    return packageContext;
}

const PackageNode* PackageNode::GetPackage() const
{
    return this;
}

bool PackageNode::IsImported() const
{
    return GetParent() != nullptr;
}

bool PackageNode::CanRemove() const
{
    return GetParent() != nullptr && !GetParent()->IsReadOnly();
}

bool PackageNode::IsReadOnly() const
{
    return GetParent() != nullptr;
}

ImportedPackagesNode* PackageNode::GetImportedPackagesNode() const
{
    return importedPackagesNode;
}

PackageControlsNode* PackageNode::GetPackageControlsNode() const
{
    return packageControlsNode;
}

PackageControlsNode* PackageNode::GetPrototypes() const
{
    return prototypes;
}

StyleSheetsNode* PackageNode::GetStyleSheets() const
{
    return styleSheets;
}

PackageNode* PackageNode::FindImportedPackage(const DAVA::FilePath& path) const
{
    for (int32 index = 0; index < importedPackagesNode->GetCount(); index++)
    {
        if (importedPackagesNode->GetImportedPackage(index)->GetPath() == path)
            return importedPackagesNode->GetImportedPackage(index);
    }
    return nullptr;
}

bool PackageNode::FindPackageInImportedPackagesRecursively(const PackageNode* node) const
{
    return FindPackageInImportedPackagesRecursively(node->GetPath());
}

bool PackageNode::FindPackageInImportedPackagesRecursively(const DAVA::FilePath& path) const
{
    for (int32 index = 0; index < importedPackagesNode->GetCount(); index++)
    {
        PackageNode* importedPackage = importedPackagesNode->GetImportedPackage(index);
        if (importedPackage->GetPath().GetFrameworkPath() == path.GetFrameworkPath())
            return true;
        if (importedPackage->FindPackageInImportedPackagesRecursively(path))
            return true;
    }
    return false;
}

void PackageNode::AddListener(PackageListener* listener)
{
    listeners.push_back(listener);
}

void PackageNode::RemoveListener(PackageListener* listener)
{
    auto it = std::find(listeners.begin(), listeners.end(), listener);
    if (it != listeners.end())
    {
        listeners.erase(it);
    }
    else
    {
        DVASSERT(false);
    }
}

void PackageNode::SetControlProperty(ControlNode* node, AbstractProperty* property, const DAVA::Any& newValue)
{
    OnControlPropertyWillBeChanged(node, property, property->GetValue(), newValue);
    node->GetRootProperty()->SetProperty(property, newValue);
    RefreshProperty(node, property);
}

void PackageNode::SetControlBindingProperty(ControlNode* node, AbstractProperty* property, const DAVA::String& newBinding, int32 bindingUpdateMode)
{
    node->GetRootProperty()->SetBindingProperty(property, newBinding, bindingUpdateMode);
    RefreshProperty(node, property);
}

void PackageNode::ResetControlProperty(ControlNode* node, AbstractProperty* property)
{
    if (property->IsOverriddenLocally())
    {
        OnControlPropertyWillBeChanged(node, property, property->GetValue(), property->GetDefaultValue());
        node->GetRootProperty()->ResetProperty(property);
        RefreshProperty(node, property);
    }
}

void PackageNode::SetControlPropertyForceOverride(ControlNode* node, ValueProperty* property, bool forceOverriden)
{
    node->GetRootProperty()->SetPropertyForceOverride(property, forceOverriden);
    RefreshProperty(node, property);
}

void PackageNode::RefreshProperty(ControlNode* node, AbstractProperty* property)
{
    node->GetRootProperty()->RefreshProperty(property, AbstractProperty::REFRESH_DEFAULT_VALUE | AbstractProperty::REFRESH_LOCALIZATION | AbstractProperty::REFRESH_FONT);

    RefreshPropertiesInInstances(node, property);

    for (PackageListener* listener : listeners)
        listener->ControlPropertyWasChanged(node, property);
}

void PackageNode::AddComponent(ControlNode* node, ComponentPropertiesSection* section)
{
    node->GetRootProperty()->AddComponentPropertiesSection(section);
    for (PackageListener* listener : listeners)
        listener->ControlComponentWasAdded(node, section);
}

void PackageNode::RemoveComponent(ControlNode* node, ComponentPropertiesSection* section)
{
    node->GetRootProperty()->RemoveComponentPropertiesSection(section);
    for (PackageListener* listener : listeners)
        listener->ControlComponentWasRemoved(node, section);
}

void PackageNode::AttachPrototypeComponent(ControlNode* node, ComponentPropertiesSection* section, ComponentPropertiesSection* prototypeSection)
{
    node->GetRootProperty()->AttachPrototypeComponent(section, prototypeSection);

    RefreshProperty(node, section);
    for (uint32 i = 0; i < section->GetCount(); i++)
        RefreshProperty(node, section->GetProperty(i));
}

void PackageNode::DetachPrototypeComponent(ControlNode* node, ComponentPropertiesSection* section, ComponentPropertiesSection* prototypeSection)
{
    node->GetRootProperty()->DetachPrototypeComponent(section, prototypeSection);

    RefreshProperty(node, section);
    for (uint32 i = 0; i < section->GetCount(); i++)
        RefreshProperty(node, section->GetProperty(i));
}

void PackageNode::SetStyleProperty(StyleSheetNode* node, AbstractProperty* property, const DAVA::Any& newValue)
{
    node->GetRootProperty()->SetProperty(property, newValue);
    node->UpdateName();

    for (PackageListener* listener : listeners)
        listener->StylePropertyWasChanged(node, property);

    RefreshPackageStylesAndLayout();
}

void PackageNode::AddStyleProperty(StyleSheetNode* node, StyleSheetProperty* property)
{
    node->GetRootProperty()->AddProperty(property);
    RefreshPackageStylesAndLayout();
}

void PackageNode::RemoveStyleProperty(StyleSheetNode* node, StyleSheetProperty* property)
{
    node->GetRootProperty()->RemoveProperty(property);
    RefreshPackageStylesAndLayout();
}

void PackageNode::InsertSelector(StyleSheetNode* node, StyleSheetSelectorProperty* property, DAVA::int32 index)
{
    node->GetRootProperty()->InsertSelector(property, index);
    node->UpdateName();

    for (PackageListener* listener : listeners)
        listener->StylePropertyWasChanged(node, property);

    RefreshPackageStylesAndLayout();
}

void PackageNode::RemoveSelector(StyleSheetNode* node, StyleSheetSelectorProperty* property)
{
    node->GetRootProperty()->RemoveSelector(property);
    node->UpdateName();

    for (PackageListener* listener : listeners)
        listener->StylePropertyWasChanged(node, property);

    RefreshPackageStylesAndLayout();
}

void PackageNode::InsertControl(ControlNode* node, ControlsContainerNode* dest, DAVA::int32 index)
{
    for (PackageListener* listener : listeners)
        listener->ControlWillBeAdded(node, dest, index);

    node->MarkAsAlive();
    dest->InsertAtIndex(index, node);

    for (PackageListener* listener : listeners)
        listener->ControlWasAdded(node, dest, index);
}

void PackageNode::RemoveControl(ControlNode* node, ControlsContainerNode* from)
{
    for (PackageListener* listener : listeners)
        listener->ControlWillBeRemoved(node, from);

    node->MarkAsRemoved();
    from->Remove(node);

    for (PackageListener* listener : listeners)
        listener->ControlWasRemoved(node, from);
}

void PackageNode::InsertStyle(StyleSheetNode* node, StyleSheetsNode* dest, DAVA::int32 index)
{
    for (PackageListener* listener : listeners)
        listener->StyleWillBeAdded(node, dest, index);

    dest->InsertAtIndex(index, node);

    for (PackageListener* listener : listeners)
        listener->StyleWasAdded(node, dest, index);

    RefreshPackageStylesAndLayout();
}

void PackageNode::RemoveStyle(StyleSheetNode* node, StyleSheetsNode* from)
{
    for (PackageListener* listener : listeners)
        listener->StyleWillBeRemoved(node, from);

    from->Remove(node);

    for (PackageListener* listener : listeners)
        listener->StyleWasRemoved(node, from);

    RefreshPackageStylesAndLayout();
}

void PackageNode::InsertImportedPackage(PackageNode* node, DAVA::int32 index)
{
    for (PackageListener* listener : listeners)
        listener->ImportedPackageWillBeAdded(node, importedPackagesNode, index);

    importedPackagesNode->InsertAtIndex(index, node);

    for (PackageListener* listener : listeners)
        listener->ImportedPackageWasAdded(node, importedPackagesNode, index);

    RefreshPackageStylesAndLayout();
}

void PackageNode::RemoveImportedPackage(PackageNode* node)
{
    for (PackageListener* listener : listeners)
        listener->ImportedPackageWillBeRemoved(node, importedPackagesNode);

    importedPackagesNode->Remove(node);

    RefreshPackageStylesAndLayout();

    for (PackageListener* listener : listeners)
        listener->ImportedPackageWasRemoved(node, importedPackagesNode);
}

void PackageNode::RebuildStyleSheets()
{
    Vector<DepthPackageNode> importedPackages = CollectImportedPackagesRecursively();

    std::sort(importedPackages.begin(), importedPackages.end(),
              [](const DepthPackageNode& a, const DepthPackageNode& b)
              {
                  return a.packageNode->GetPath() == b.packageNode->GetPath() ?
                  a.depth < b.depth :
                  a.packageNode->GetPath() < b.packageNode->GetPath();
              });

    auto lastNeeded = std::unique(importedPackages.begin(), importedPackages.end(),
                                  [](const DepthPackageNode& a, const DepthPackageNode& b)
                                  {
                                      return a.packageNode->GetPath() == b.packageNode->GetPath();
                                  });

    importedPackages.erase(lastNeeded, importedPackages.end());

    packageContext->RemoveAllStyleSheets();

    for (const DepthPackageNode& importedPackage : importedPackages)
    {
        StyleSheetsNode* styleSheetsNode = importedPackage.packageNode->GetStyleSheets();
        for (int32 i = 0; i < styleSheetsNode->GetCount(); i++)
        {
            StyleSheetNode* node = styleSheetsNode->Get(i);
            Vector<UIStyleSheet*> styleSheets = node->GetRootProperty()->CollectStyleSheets();
            for (UIStyleSheet* styleSheet : styleSheets)
            {
                packageContext->AddStyleSheet(UIPriorityStyleSheet(styleSheet, importedPackage.depth));
            }
        }
    }

    for (PackageListener* listener : listeners)
    {
        listener->StyleSheetsWereRebuilt();
    }
}

void PackageNode::RefreshPackageStylesAndLayout(bool includeImportedPackages)
{
    if (includeImportedPackages)
    {
        for (int32 i = 0; i < importedPackagesNode->GetCount(); i++)
        {
            importedPackagesNode->GetImportedPackage(i)->RefreshPackageStylesAndLayout(true);
        }
    }

    RebuildStyleSheets();

    for (int32 i = 0; i < packageControlsNode->GetCount(); i++)
    {
        GetEngineContext()->uiControlSystem->GetStyleSheetSystem()->ProcessControl(packageControlsNode->Get(i)->GetControl(), true);
        packageControlsNode->Get(i)->GetControl()->SetLayoutDirty();
        NotifyPropertyChanged(packageControlsNode->Get(i));
    }
    for (int32 i = 0; i < prototypes->GetCount(); i++)
    {
        GetEngineContext()->uiControlSystem->GetStyleSheetSystem()->ProcessControl(prototypes->Get(i)->GetControl(), true);
        prototypes->Get(i)->GetControl()->SetLayoutDirty();
        NotifyPropertyChanged(prototypes->Get(i));
    }
}

void PackageNode::SetCanUpdateAll(bool canUpdate)
{
    canUpdateAll = canUpdate;
}

bool PackageNode::CanUpdateAll() const
{
    return canUpdateAll;
}

PackageNode::AxisGuides PackageNode::GetAxisGuides(const DAVA::String& name, DAVA::Vector2::eAxis orientation)
{
    auto iter = allGuides.find(name);
    if (iter != allGuides.end())
    {
        return iter->second[orientation];
    }
    return AxisGuides();
}

void PackageNode::SetAxisGuides(const DAVA::String& name, DAVA::Vector2::eAxis orientation, const PackageNode::AxisGuides& guidesValues)
{
    DVASSERT(name.empty() == false);
    Guides& guides = allGuides[name];
    guides[orientation] = guidesValues;
}

PackageNode::Guides PackageNode::GetGuides(const DAVA::String& name) const
{
    auto iter = allGuides.find(name);
    if (iter != allGuides.end())
    {
        return iter->second;
    }
    return Guides();
}

bool PackageNode::HasCustomData() const
{
    for (const auto& mapItem : allGuides)
    {
        const Guides& guides = mapItem.second;
        if (guides[Vector2::AXIS_X].empty() == false || guides[Vector2::AXIS_Y].empty() == false)
        {
            return true;
        }
    }
    return false;
}

void PackageNode::SetGuides(const DAVA::String& name, const PackageNode::Guides& guides)
{
    allGuides[name] = guides;
}

void PackageNode::RefreshPropertiesInInstances(ControlNode* node, AbstractProperty* property)
{
    for (ControlNode* instance : node->GetInstances())
    {
        AbstractProperty* instanceProperty = instance->GetRootProperty()->FindPropertyByPrototype(property);
        if (instanceProperty)
            RefreshProperty(instance, instanceProperty);
    }
}

void PackageNode::NotifyPropertyChanged(ControlNode* control)
{
    RootProperty* rootProperty = control->GetRootProperty();

    for (int32 i = 0; i < rootProperty->GetControlPropertiesSectionsCount(); i++)
    {
        ControlPropertiesSection* controlSection = rootProperty->GetControlPropertiesSection(i);
        for (uint32 j = 0; j < controlSection->GetCount(); j++)
        {
            AbstractProperty* prop = controlSection->GetProperty(j);
            for (PackageListener* listener : listeners)
                listener->ControlPropertyWasChanged(control, prop);
        }
    }

    for (int i = 0; i < control->GetCount(); i++)
    {
        NotifyPropertyChanged(control->Get(i));
    }
}

Vector<PackageNode::DepthPackageNode> PackageNode::CollectImportedPackagesRecursively()
{
    Vector<DepthPackageNode> result;

    result.push_back(DepthPackageNode(0, this));

    // collect imported packages recursively

    for (int32 packageIndex = 0; packageIndex < result.size(); ++packageIndex)
    {
        PackageNode* packageNode = result[packageIndex].packageNode;
        int32 depth = result[packageIndex].depth;

        ImportedPackagesNode* importedPackagesNode = packageNode->GetImportedPackagesNode();
        for (int32 i = 0; i < importedPackagesNode->GetCount(); i++)
        {
            PackageNode* node = importedPackagesNode->GetImportedPackage(i);
            result.push_back(DepthPackageNode(depth + 1, node));
        }
    }

    return result;
}

void PackageNode::OnControlPropertyWillBeChanged(ControlNode* node, AbstractProperty* property, const Any& oldValue, const Any& newValue)
{
    using namespace DAVA;
    DVASSERT(node != nullptr);
    DVASSERT(property != nullptr);

    if (dynamic_cast<PackageControlsNode*>(node->GetParent()) != nullptr && property->GetName() == "Name")
    {
        String name = oldValue.Cast<String>(String());
        Guides guides = GetGuides(name);

        if (FindRootWithSameName(node, this) == false)
        {
            SetGuides(name, Guides());
        }

        String newName = newValue.Cast<String>(String());
        //we don't support root controls without name
        //all notification messages must be separate from this logic
        if (newName.empty() == false)
        {
            SetGuides(newName, guides);
        }
    }
}

bool FindRootWithSameName(ControlNode* control, PackageNode* package)
{
    DVASSERT(dynamic_cast<PackageControlsNode*>(control->GetParent()) != nullptr);
    String name = control->GetName();
    Vector<PackageControlsNode*> rootControlsHolders = { package->GetPrototypes(), package->GetPackageControlsNode() };

    for (PackageControlsNode* rootControlsHolder : rootControlsHolders)
    {
        for (int i = 0, count = rootControlsHolder->GetCount(); i < count; i++)
        {
            ControlNode* rootChild = rootControlsHolder->Get(i);
            if (control != rootChild && control->GetName() == rootChild->GetName())
            {
                return true;
            }
        }
    }
    return false;
}

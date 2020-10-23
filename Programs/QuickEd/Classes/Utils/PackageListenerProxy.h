#pragma once

#include "Model/PackageHierarchy/PackageNode.h"
#include "Model/PackageHierarchy/PackageListener.h"

#include <Base/RefPtr.h>

#include <memory>

namespace DAVA
{
class Any;
class ContextAccessor;
class FieldBinder;
}

class PackageListenerProxy : public PackageListener
{
public:
    PackageListenerProxy();
    PackageListenerProxy(PackageListener* listener, DAVA::ContextAccessor* accessor);
    ~PackageListenerProxy();

    void Init(PackageListener* listener, DAVA::ContextAccessor* accessor);

private:
    void OnPackageChanged(const DAVA::Any& package);

    void ActivePackageNodeWasChanged(PackageNode* node) override;

    void ControlPropertyWasChanged(ControlNode* node, AbstractProperty* property) override;
    void StylePropertyWasChanged(StyleSheetNode* node, AbstractProperty* property) override;
    void ControlComponentWasAdded(ControlNode* node, ComponentPropertiesSection* section) override;
    void ControlComponentWasRemoved(ControlNode* node, ComponentPropertiesSection* section) override;

    void ControlWillBeAdded(ControlNode* node, ControlsContainerNode* destination, int index) override;
    void ControlWasAdded(ControlNode* node, ControlsContainerNode* destination, int index) override;

    void ControlWillBeRemoved(ControlNode* node, ControlsContainerNode* from) override;
    void ControlWasRemoved(ControlNode* node, ControlsContainerNode* from) override;

    void StyleWillBeAdded(StyleSheetNode* node, StyleSheetsNode* destination, int index) override;
    void StyleWasAdded(StyleSheetNode* node, StyleSheetsNode* destination, int index) override;

    void StyleWillBeRemoved(StyleSheetNode* node, StyleSheetsNode* from) override;
    void StyleWasRemoved(StyleSheetNode* node, StyleSheetsNode* from) override;

    void ImportedPackageWillBeAdded(PackageNode* node, ImportedPackagesNode* to, int index) override;
    void ImportedPackageWasAdded(PackageNode* node, ImportedPackagesNode* to, int index) override;

    void ImportedPackageWillBeRemoved(PackageNode* node, ImportedPackagesNode* from) override;
    void ImportedPackageWasRemoved(PackageNode* node, ImportedPackagesNode* from) override;

    void StyleSheetsWereRebuilt() override;

    DAVA::RefPtr<PackageNode> package;
    PackageListener* listener;
    std::unique_ptr<DAVA::FieldBinder> fieldBinder;
};

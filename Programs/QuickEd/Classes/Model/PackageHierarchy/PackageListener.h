#pragma once

class PackageNode;
class ControlNode;
class ControlsContainerNode;
class StyleSheetNode;
class StyleSheetsNode;
class PackageControlsNode;
class AbstractProperty;
class ImportedPackagesNode;
class ComponentPropertiesSection;

class PackageListener
{
public:
    virtual ~PackageListener(){};

    virtual void ActivePackageNodeWasChanged(PackageNode* node){};

    virtual void ControlPropertyWasChanged(ControlNode* node, AbstractProperty* property){};
    virtual void StylePropertyWasChanged(StyleSheetNode* node, AbstractProperty* property){};
    virtual void ControlComponentWasAdded(ControlNode* node, ComponentPropertiesSection* section){};
    virtual void ControlComponentWasRemoved(ControlNode* node, ComponentPropertiesSection* section){};

    virtual void ControlWillBeAdded(ControlNode* node, ControlsContainerNode* destination, int index){};
    virtual void ControlWasAdded(ControlNode* node, ControlsContainerNode* destination, int index){};

    virtual void ControlWillBeRemoved(ControlNode* node, ControlsContainerNode* from){};
    virtual void ControlWasRemoved(ControlNode* node, ControlsContainerNode* from){};

    virtual void StyleWillBeAdded(StyleSheetNode* node, StyleSheetsNode* destination, int index){};
    virtual void StyleWasAdded(StyleSheetNode* node, StyleSheetsNode* destination, int index){};

    virtual void StyleWillBeRemoved(StyleSheetNode* node, StyleSheetsNode* from){};
    virtual void StyleWasRemoved(StyleSheetNode* node, StyleSheetsNode* from){};

    virtual void ImportedPackageWillBeAdded(PackageNode* node, ImportedPackagesNode* to, int index){};
    virtual void ImportedPackageWasAdded(PackageNode* node, ImportedPackagesNode* to, int index){};

    virtual void ImportedPackageWillBeRemoved(PackageNode* node, ImportedPackagesNode* from){};
    virtual void ImportedPackageWasRemoved(PackageNode* node, ImportedPackagesNode* from){};

    virtual void StyleSheetsWereRebuilt(){};
};

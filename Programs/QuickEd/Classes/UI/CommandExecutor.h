#pragma once

#include "EditorSystems/EditorSystemsManager.h"
#include "EditorSystems/SelectionContainer.h"

#include <TArc/DataProcessing/DataContext.h>
#include <TArc/Qt/QtString.h>

#include <Command/Command.h>
#include <Base/BaseObject.h>
#include <Base/Result.h>

namespace DAVA
{
class CommandStack;
class ContextAccessor;
class UI;
}

class DocumentData;
class ProjectData;
class PackageBaseNode;

class ControlNode;
class StyleSheetNode;
class StyleSheetsNode;
class PackageControlsNode;
class PackageNode;
class AbstractProperty;
class ControlsContainerNode;
class ComponentPropertiesSection;

class CommandExecutor
{
public:
    CommandExecutor(DAVA::ContextAccessor* accessor, DAVA::UI* ui);

    void AddImportedPackagesIntoPackage(const DAVA::Vector<DAVA::FilePath> packagePaths, const PackageNode* package);
    void RemoveImportedPackagesFromPackage(const DAVA::Vector<PackageNode*>& importedPackage, const PackageNode* package);

    void ChangeProperty(ControlNode* node, AbstractProperty* property, const DAVA::Any& value);
    void ChangeBindingProperty(ControlNode* node, AbstractProperty* property, const DAVA::String& value, DAVA::int32 mode);
    void ResetProperty(ControlNode* node, AbstractProperty* property);

    void AddComponent(ControlNode* node, const DAVA::Type* componentType);
    void RemoveComponent(ControlNode* node, const DAVA::Type* componentType, DAVA::uint32 componentIndex);

    void ChangeProperty(StyleSheetNode* node, AbstractProperty* property, const DAVA::Any& value);

    void AddStyleProperty(StyleSheetNode* node, DAVA::uint32 propertyIndex);
    void RemoveStyleProperty(StyleSheetNode* node, DAVA::uint32 propertyIndex);

    void AddStyleSelector(StyleSheetNode* node);
    void RemoveStyleSelector(StyleSheetNode* node, DAVA::int32 selectorIndex);

    void InsertControl(ControlNode* control, ControlsContainerNode* dest, DAVA::int32 destIndex) const;
    DAVA::Vector<ControlNode*> InsertInstances(const DAVA::Vector<ControlNode*>& controls, ControlsContainerNode* dest, DAVA::int32 destIndex);
    DAVA::Vector<ControlNode*> CopyControls(const DAVA::Vector<ControlNode*>& nodes, ControlsContainerNode* dest, DAVA::int32 destIndex);
    DAVA::Vector<ControlNode*> MoveControls(const DAVA::Vector<ControlNode*>& nodes, ControlsContainerNode* dest, DAVA::int32 destIndex) const;

    DAVA::ResultList InsertStyle(StyleSheetNode* node, StyleSheetsNode* dest, DAVA::int32 destIndex);
    void CopyStyles(const DAVA::Vector<StyleSheetNode*>& nodes, StyleSheetsNode* dest, DAVA::int32 destIndex);
    void MoveStyles(const DAVA::Vector<StyleSheetNode*>& nodes, StyleSheetsNode* dest, DAVA::int32 destIndex);

    void Remove(const DAVA::Vector<ControlNode*>& controls, const DAVA::Vector<StyleSheetNode*>& styles) const;
    SelectedNodes Paste(PackageNode* root, PackageBaseNode* dest, DAVA::int32 destIndex, const DAVA::String& data);

private:
    void AddImportedPackageIntoPackageImpl(PackageNode* importedPackage, const PackageNode* package);
    void InsertControlImpl(ControlNode* control, ControlsContainerNode* dest, DAVA::int32 destIndex) const;
    void RemoveControlImpl(ControlNode* node) const;
    bool MoveControlImpl(ControlNode* node, ControlsContainerNode* dest, DAVA::int32 destIndex) const;
    void AddComponentImpl(ControlNode* node, const DAVA::Type* type, DAVA::int32 index, ComponentPropertiesSection* prototypeSection);
    void RemoveComponentImpl(ControlNode* node, ComponentPropertiesSection* section);
    bool IsNodeInHierarchy(const PackageBaseNode* node) const;

    DocumentData* GetDocumentData() const;
    ProjectData* GetProjectData() const;

    DAVA::ContextAccessor* accessor = nullptr;
    DAVA::UI* ui = nullptr;
};

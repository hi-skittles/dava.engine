#pragma once

#include "UI/AbstractUIPackageBuilder.h"
#include "FileSystem/FilePath.h"
#include "UI/Styles/UIStyleSheetStructs.h"
#include "Model/ControlProperties/SectionProperty.h"
#include "Model/PackageHierarchy/PackageNode.h"

#include <Base/Result.h>
#include <Engine/Engine.h>

class ControlNode;
class StyleSheetNode;
class ControlsContainerNode;
class IntrospectionProperty;

class QuickEdPackageBuilder : public DAVA::AbstractUIPackageBuilder
{
public:
    QuickEdPackageBuilder(const DAVA::EngineContext* engineContext);
    virtual ~QuickEdPackageBuilder();

    virtual void BeginPackage(const DAVA::FilePath& packagePath, DAVA::int32 version) override;
    virtual void EndPackage() override;

    bool ProcessImportedPackage(const DAVA::String& packagePath, DAVA::AbstractUIPackageLoader* loader) override;
    void ProcessStyleSheet(const DAVA::Vector<DAVA::UIStyleSheetSelectorChain>& selectorChains, const DAVA::Vector<DAVA::UIStyleSheetProperty>& properties) override;

    virtual const DAVA::ReflectedType* BeginControlWithClass(const DAVA::FastName& controlName, const DAVA::String& className) override;
    virtual const DAVA::ReflectedType* BeginControlWithCustomClass(const DAVA::FastName& controlName, const DAVA::String& customClassName, const DAVA::String& className) override;
    virtual const DAVA::ReflectedType* BeginControlWithPrototype(const DAVA::FastName& controlName, const DAVA::String& packageName, const DAVA::FastName& prototypeName, const DAVA::String* customClassName, DAVA::AbstractUIPackageLoader* loader) override;
    virtual const DAVA::ReflectedType* BeginControlWithPath(const DAVA::String& pathName) override;
    virtual const DAVA::ReflectedType* BeginUnknownControl(const DAVA::FastName& controlName, const DAVA::YamlNode* node) override;
    virtual void EndControl(eControlPlace controlPlace) override;

    void BeginControlPropertiesSection(const DAVA::String& name) override;
    void EndControlPropertiesSection() override;

    virtual const DAVA::ReflectedType* BeginComponentPropertiesSection(const DAVA::Type* componentType, DAVA::uint32 componentIndex) override;
    virtual void EndComponentPropertiesSection() override;

    void ProcessProperty(const DAVA::ReflectedStructure::Field& field, const DAVA::Any& value) override;
    void ProcessDataBinding(const DAVA::String& fieldName, const DAVA::String& expression, DAVA::int32 bindingMode) override;

    virtual void ProcessCustomData(const DAVA::YamlNode* customDataNode) override;
    void ProcessGuides(const DAVA::YamlNode* guidesNode);

    DAVA::RefPtr<PackageNode> BuildPackage() const;
    const DAVA::Vector<ControlNode*>& GetRootControls() const;
    const DAVA::Vector<PackageNode*>& GetImportedPackages() const;
    const DAVA::Vector<StyleSheetNode*>& GetStyles() const;

    void AddImportedPackage(PackageNode* node);

    const DAVA::ResultList& GetResults() const;

private:
    ControlNode* FindPrototype(const DAVA::FastName& name) const;

    struct ControlDescr
    {
        ControlNode* node;
        bool addToParent;

        ControlDescr();
        ControlDescr(ControlNode* node, bool addToParent);
        ControlDescr(const ControlDescr& descr);
        ~ControlDescr();
        ControlDescr& operator=(const ControlDescr& descr);
    };

    DAVA::FilePath packagePath;
    DAVA::List<ControlDescr> controlsStack;

    DAVA::Vector<PackageNode*> importedPackages;
    DAVA::Vector<ControlNode*> rootControls;
    DAVA::Vector<ControlNode*> prototypes;
    DAVA::Vector<StyleSheetNode*> styleSheets;
    DAVA::Vector<DAVA::FilePath> declinedPackages;

    DAVA::Map<DAVA::String, PackageNode::Guides> allGuides;

    DAVA::BaseObject* currentObject;
    SectionProperty<IntrospectionProperty>* currentSection;
    const DAVA::EngineContext* engineContext;

    DAVA::ResultList results;
};

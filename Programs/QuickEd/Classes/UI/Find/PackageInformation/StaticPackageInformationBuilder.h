#pragma once

#include "Model/ControlProperties/SectionProperty.h"
#include "UI/Find/PackageInformation/StaticPackageInformation.h"
#include "UI/Find/PackageInformation/StaticControlInformation.h"

#include <FileSystem/FilePath.h>
#include <UI/AbstractUIPackageBuilder.h>
#include <UI/Styles/UIStyleSheetStructs.h>

class PackageInformationCache
{
public:
    void Put(const std::shared_ptr<StaticPackageInformation>& package);
    std::shared_ptr<StaticPackageInformation> Find(const DAVA::String& path);

private:
    DAVA::Map<DAVA::String, std::shared_ptr<StaticPackageInformation>> packages;
};

class StaticPackageInformationBuilder : public DAVA::AbstractUIPackageBuilder
{
public:
    StaticPackageInformationBuilder(PackageInformationCache* cache);
    ~StaticPackageInformationBuilder() override;

    void BeginPackage(const DAVA::FilePath& packagePath, DAVA::int32 version) override;
    void EndPackage() override;

    bool ProcessImportedPackage(const DAVA::String& packagePath, DAVA::AbstractUIPackageLoader* loader) override;
    void ProcessStyleSheet(const DAVA::Vector<DAVA::UIStyleSheetSelectorChain>& selectorChains, const DAVA::Vector<DAVA::UIStyleSheetProperty>& properties) override;

    const DAVA::ReflectedType* BeginControlWithClass(const DAVA::FastName& controlName, const DAVA::String& className) override;
    const DAVA::ReflectedType* BeginControlWithCustomClass(const DAVA::FastName& controlName, const DAVA::String& customClassName, const DAVA::String& className) override;
    const DAVA::ReflectedType* BeginControlWithPrototype(const DAVA::FastName& controlName, const DAVA::String& packageName, const DAVA::FastName& prototypeName, const DAVA::String* customClassName, DAVA::AbstractUIPackageLoader* loader) override;
    const DAVA::ReflectedType* BeginControlWithPath(const DAVA::String& pathName) override;
    const DAVA::ReflectedType* BeginUnknownControl(const DAVA::FastName& controlName, const DAVA::YamlNode* node) override;
    void EndControl(eControlPlace controlPlace) override;

    void BeginControlPropertiesSection(const DAVA::String& name) override;
    void EndControlPropertiesSection() override;

    const DAVA::ReflectedType* BeginComponentPropertiesSection(const DAVA::Type*, DAVA::uint32 componentIndex) override;
    void EndComponentPropertiesSection() override;

    void ProcessProperty(const DAVA::ReflectedStructure::Field& field, const DAVA::Any& value) override;
    void ProcessDataBinding(const DAVA::String& fieldName, const DAVA::String& expression, DAVA::int32 bindingMode) override;

    std::shared_ptr<StaticPackageInformation> GetPackage() const;

private:
    struct Description;

    std::shared_ptr<StaticPackageInformation> packageInformation;
    DAVA::Vector<Description> stack;
    DAVA::Function<void(const DAVA::ReflectedStructure::Field& member, const DAVA::Any& value)> propertyProcessor;

    PackageInformationCache* cache = nullptr;
    DAVA::ResultList results;
};

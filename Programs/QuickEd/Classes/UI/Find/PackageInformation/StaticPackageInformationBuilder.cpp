#include "UI/Find/PackageInformation/StaticPackageInformationBuilder.h"

#include <Reflection/ReflectedTypeDB.h>
#include <UI/UIControl.h>
#include <Utils/Utils.h>
#include <Utils/StringFormat.h>

using namespace DAVA;

struct StaticPackageInformationBuilder::Description
{
    std::shared_ptr<StaticControlInformation> controlInformation;

    bool addToParent = false;

    Description(const std::shared_ptr<StaticControlInformation>& controlInformation_, bool addToParent_)
        : controlInformation(controlInformation_)
        , addToParent(addToParent_)
    {
    }
};

void PackageInformationCache::Put(const std::shared_ptr<StaticPackageInformation>& package)
{
    DVASSERT(packages.find(package->GetPath()) == packages.end());
    packages[package->GetPath()] = package;
}

std::shared_ptr<StaticPackageInformation> PackageInformationCache::Find(const DAVA::String& path)
{
    auto it = packages.find(path);
    if (it != packages.end())
    {
        return it->second;
    }

    return std::shared_ptr<StaticPackageInformation>();
}

StaticPackageInformationBuilder::StaticPackageInformationBuilder(PackageInformationCache* cache_)
    : cache(cache_)
{
}

StaticPackageInformationBuilder::~StaticPackageInformationBuilder()
{
}

void StaticPackageInformationBuilder::BeginPackage(const DAVA::FilePath& packagePath, int32 version)
{
    DVASSERT(packageInformation.get() == nullptr);
    packageInformation = std::make_shared<StaticPackageInformation>(packagePath.GetFrameworkPath(), version);
}

void StaticPackageInformationBuilder::EndPackage()
{
}

bool StaticPackageInformationBuilder::ProcessImportedPackage(const DAVA::String& packagePathStr, DAVA::AbstractUIPackageLoader* loader)
{
    std::shared_ptr<StaticPackageInformation> pack = cache->Find(packagePathStr);
    if (pack != nullptr)
    {
        packageInformation->AddImportedPackage(pack);
        return true;
    }
    else
    {
        FilePath packagePath(packagePathStr);
        StaticPackageInformationBuilder builder(cache);

        if (loader->LoadPackage(packagePath, &builder))
        {
            pack = builder.GetPackage();

            cache->Put(pack);
            packageInformation->AddImportedPackage(pack);
            return true;
        }
        else
        {
            Result r(Result::RESULT_ERROR, Format("Can't import package '%s'", packagePathStr.c_str()));
            results.AddResult(r);
            packageInformation->AddResult(r);
            return false;
        }
    }

    DVASSERT(false);
    return false;
}

void StaticPackageInformationBuilder::ProcessStyleSheet(const DAVA::Vector<DAVA::UIStyleSheetSelectorChain>& selectorChains, const DAVA::Vector<DAVA::UIStyleSheetProperty>& properties)
{
    // do nothing
}

const ReflectedType* StaticPackageInformationBuilder::BeginControlWithClass(const FastName& controlName, const DAVA::String& className)
{
    stack.emplace_back(Description(std::make_shared<StaticControlInformation>(controlName), true));
    return ReflectedTypeDB::Get<UIControl>();
}

const ReflectedType* StaticPackageInformationBuilder::BeginControlWithCustomClass(const FastName& controlName, const DAVA::String& customClassName, const DAVA::String& className)
{
    stack.emplace_back(Description(std::make_shared<StaticControlInformation>(controlName), true));

    return ReflectedTypeDB::Get<UIControl>();
}

const ReflectedType* StaticPackageInformationBuilder::BeginControlWithPrototype(const FastName& controlName, const DAVA::String& packageName, const DAVA::FastName& prototypeName, const DAVA::String* customClassName, DAVA::AbstractUIPackageLoader* loader)
{
    std::shared_ptr<StaticPackageInformation> prototypePackage;
    std::shared_ptr<StaticControlInformation> prototype;

    if (packageName.empty())
    {
        prototypePackage = packageInformation;
        prototype = packageInformation->FindPrototypeByName(prototypeName);

        if (prototype.get() == nullptr)
        {
            if (loader->LoadControlByName(prototypeName, this))
            {
                prototype = packageInformation->FindPrototypeByName(prototypeName);
            }
        }
    }
    else
    {
        const DAVA::Vector<std::shared_ptr<StaticPackageInformation>>& packages = packageInformation->GetImportedPackages();
        auto it = std::find_if(packages.begin(), packages.end(), [packageName](const std::shared_ptr<StaticPackageInformation>& pack) {
            return FilePath(pack->GetPath()).GetBasename() == packageName;
        });

        if (it != packages.end())
        {
            prototypePackage = *it;
            prototype = prototypePackage->FindPrototypeByName(prototypeName);
        }
    }

    if (prototype && prototypePackage)
    {
        stack.emplace_back(std::make_shared<StaticControlInformation>(*prototype, controlName, prototypePackage, FastName(prototypeName)), true);
    }
    else
    {
        Description d(std::make_shared<StaticControlInformation>(controlName), true);
        String errorMsg = Format("Can't find prototype '%s' from package '%s'",
                                 prototypeName.c_str(), packageName.c_str());
        d.controlInformation->AddResult(Result(Result::RESULT_ERROR, errorMsg));
        stack.push_back(d);
    }
    return ReflectedTypeDB::Get<UIControl>();
}

const ReflectedType* StaticPackageInformationBuilder::BeginControlWithPath(const DAVA::String& pathName)
{
    if (!stack.empty())
    {
        std::shared_ptr<StaticControlInformation> ptr = stack.back().controlInformation;

        Vector<String> controlNames;
        Split(pathName, "/", controlNames, false, true);
        for (String& name : controlNames)
        {
            FastName childName(name);
            std::shared_ptr<StaticControlInformation> child = ptr->FindChildByName(childName);
            if (!child)
            {
                results.AddResult(Result(Result::RESULT_ERROR, Format("Access to removed control by path '%s'", pathName.c_str())));

                child = std::make_shared<StaticControlInformation>(childName);
                child->SetParent(ptr.get());
                child->AddResult(Result(Result::RESULT_ERROR, "Control was removed in prototype"));
                ptr->AddChild(child);
            }

            ptr = child;
        }

        stack.emplace_back(Description(ptr, false));
    }
    else
    {
        DVASSERT(false);
    }

    return ReflectedTypeDB::Get<UIControl>();
}

const ReflectedType* StaticPackageInformationBuilder::BeginUnknownControl(const FastName& controlName, const DAVA::YamlNode* node)
{
    stack.emplace_back(Description(std::make_shared<StaticControlInformation>(controlName), true));
    return ReflectedTypeDB::Get<UIControl>();
}

void StaticPackageInformationBuilder::EndControl(eControlPlace controlPlace)
{
    Description descr = stack.back();
    stack.pop_back();

    if (descr.addToParent)
    {
        switch (controlPlace)
        {
        case TO_CONTROLS:
            packageInformation->AddControl(descr.controlInformation);
            break;

        case TO_PROTOTYPES:
            packageInformation->AddPrototype(descr.controlInformation);
            break;

        case TO_PREVIOUS_CONTROL:
            DVASSERT(!stack.empty());
            descr.controlInformation->SetParent(stack.back().controlInformation.get());
            stack.back().controlInformation->AddChild(descr.controlInformation);
            break;

        default:
            DVASSERT(false);
            break;
        }
    }
}

void StaticPackageInformationBuilder::BeginControlPropertiesSection(const DAVA::String& name)
{
    std::shared_ptr<StaticControlInformation> ptr = stack.back().controlInformation;

    propertyProcessor = [ptr](const DAVA::ReflectedStructure::Field& member, const DAVA::Any& value)
    {
        ptr->SetControlProperty(member, value);
    };
}

void StaticPackageInformationBuilder::EndControlPropertiesSection()
{
    propertyProcessor = nullptr;
}

const ReflectedType* StaticPackageInformationBuilder::BeginComponentPropertiesSection(const DAVA::Type* componentType, DAVA::uint32 componentIndex)
{
    std::shared_ptr<StaticControlInformation> ptr = stack.back().controlInformation;

    ptr->AddComponent(componentType);

    propertyProcessor = [ptr, componentType, componentIndex](const DAVA::ReflectedStructure::Field& member, const DAVA::Any& value)
    {
        ptr->SetComponentProperty(componentType, componentIndex, member, value);
    };

    RefPtr<UIComponent> component = UIComponent::SafeCreateByType(componentType); // this will be gone with new reflection system

    return ReflectedTypeDB::GetByPointer(component.Get());
}

void StaticPackageInformationBuilder::EndComponentPropertiesSection()
{
    propertyProcessor = nullptr;
}

void StaticPackageInformationBuilder::ProcessProperty(const DAVA::ReflectedStructure::Field& field, const DAVA::Any& value)
{
    if (!value.IsEmpty())
    {
        propertyProcessor(field, value);
    }
}

void StaticPackageInformationBuilder::ProcessDataBinding(const DAVA::String& fieldName, const DAVA::String& expression, DAVA::int32 bindingMode)
{
}

std::shared_ptr<StaticPackageInformation> StaticPackageInformationBuilder::GetPackage() const
{
    return packageInformation;
}

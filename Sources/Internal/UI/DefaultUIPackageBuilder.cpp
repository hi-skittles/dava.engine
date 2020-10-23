#include "DefaultUIPackageBuilder.h"

#include "Engine/Engine.h"
#include "UI/UIPackage.h"
#include "UI/UIPackageLoader.h"
#include "UI/UIControlSystem.h"
#include "UI/Layouts/UILayoutSystem.h"

#include "Base/ObjectFactory.h"
#include "UI/UIControl.h"
#include "UI/UIControlHelpers.h"
#include "UI/DataBinding/UIDataBindingComponent.h"
#include "UI/Components/UIComponent.h"
#include "FileSystem/LocalizationSystem.h"
#include "UI/UIPackagesCache.h"
#include "UI/Styles/UIStyleSheet.h"
#include "Styles/UIStyleSheetSystem.h"

#include "Logger/Logger.h"
#include "Reflection/ReflectionRegistrator.h"

namespace DAVA
{
namespace
{
const String EXCEPTION_CLASS_UI_TEXT_FIELD = "UITextField";
const String EXCEPTION_CLASS_UI_LIST = "UIList";
const FastName PROPERTY_NAME_TEXT("text");
}

////////////////////////////////////////////////////////////////////////////////
// ControlDescr
////////////////////////////////////////////////////////////////////////////////

struct DefaultUIPackageBuilder::ControlDescr
{
    RefPtr<UIControl> control;
    bool addToParent;

    ControlDescr(UIControl* aControl, bool aAddToParent)
        : control(RefPtr<UIControl>::ConstructWithRetain(aControl))
        , addToParent(aAddToParent)
    {
    }
};

DefaultUIPackageBuilder::DefaultUIPackageBuilder(const RefPtr<UIPackagesCache>& _packagesCache)
{
    if (_packagesCache)
        cache = _packagesCache;
    else
        cache = MakeRef<UIPackagesCache>();
}

DefaultUIPackageBuilder::DefaultUIPackageBuilder(UIPackagesCache* _packagesCache)
    : DefaultUIPackageBuilder(RefPtr<UIPackagesCache>::ConstructWithRetain(_packagesCache))
{
}

DefaultUIPackageBuilder::~DefaultUIPackageBuilder()
{
    cache = nullptr;

    if (!controlsStack.empty())
    {
        controlsStack.clear();
        DVASSERT(false);
    }

    importedPackages.clear();
}

UIPackage* DefaultUIPackageBuilder::GetPackage() const
{
    return package.Get();
}

RefPtr<UIPackage> DefaultUIPackageBuilder::FindInCache(const String& packagePath) const
{
    return cache->GetPackage(packagePath);
}

void DefaultUIPackageBuilder::BeginPackage(const FilePath& packagePath, int32 version)
{
    DVASSERT(!package.Valid());
    package = RefPtr<UIPackage>(new UIPackage());
    currentPackagePath = packagePath;
}

void DefaultUIPackageBuilder::EndPackage()
{
    for (const auto& importedPackage : importedPackages)
    {
        const Vector<UIPriorityStyleSheet>& packageStyleSheets = importedPackage->GetControlPackageContext()->GetSortedStyleSheets();
        for (const UIPriorityStyleSheet& packageStyleSheet : packageStyleSheets)
        {
            styleSheets.emplace_back(UIPriorityStyleSheet(packageStyleSheet.GetStyleSheet(), packageStyleSheet.GetPriority() + 1));
        }
    }

    // kill duplicates
    {
        std::sort(styleSheets.begin(), styleSheets.end(), [](const UIPriorityStyleSheet& a, const UIPriorityStyleSheet& b)
                  {
                      const UIStyleSheet* s1 = a.GetStyleSheet();
                      const UIStyleSheet* s2 = b.GetStyleSheet();
                      return s1 == s2 ? a.GetPriority() < b.GetPriority() : s1 < s2;
                  });
        auto lastNeeded = std::unique(styleSheets.begin(), styleSheets.end(), [](const UIPriorityStyleSheet& a, const UIPriorityStyleSheet& b)
                                      {
                                          return a.GetStyleSheet() == b.GetStyleSheet();
                                      });
        styleSheets.erase(lastNeeded, styleSheets.end());
    }

    for (const UIPriorityStyleSheet& styleSheet : styleSheets)
    {
        package->GetControlPackageContext()->AddStyleSheet(styleSheet);
    }

    styleSheets.clear();
}

bool DefaultUIPackageBuilder::ProcessImportedPackage(const String& packagePath, AbstractUIPackageLoader* loader)
{
    RefPtr<UIPackage> importedPackage = cache->GetPackage(packagePath);

    if (!importedPackage)
    {
        std::unique_ptr<DefaultUIPackageBuilder> builder = CreateBuilder(cache.Get());
        if (loader->LoadPackage(packagePath, builder.get()) && builder->GetPackage())
        {
            importedPackage = builder->GetPackage();
            cache->PutPackage(packagePath, importedPackage);
        }
    }

    if (importedPackage)
    {
        PutImportredPackage(packagePath, importedPackage);
        return true;
    }
    else
    {
        DVASSERT(false);
        return false;
    }
}

void DefaultUIPackageBuilder::ProcessStyleSheet(const Vector<UIStyleSheetSelectorChain>& selectorChains, const Vector<UIStyleSheetProperty>& properties)
{
    for (const UIStyleSheetSelectorChain& chain : selectorChains)
    {
        ScopedPtr<UIStyleSheet> styleSheet(new UIStyleSheet());
        styleSheet->SetSelectorChain(chain);
        ScopedPtr<UIStyleSheetPropertyTable> propertiesTable(new UIStyleSheetPropertyTable());
        propertiesTable->SetProperties(properties);
        styleSheet->SetPropertyTable(propertiesTable);
        styleSheet->SetSourceInfo(UIStyleSheetSourceInfo(currentPackagePath));

        package->GetControlPackageContext()->AddStyleSheet(UIPriorityStyleSheet(styleSheet));
    }
}

const ReflectedType* DefaultUIPackageBuilder::BeginControlWithClass(const FastName& controlName, const String& className)
{
    RefPtr<UIControl> control(CreateControlByName(className, className));

    if (control.Valid())
    {
        if (className != EXCEPTION_CLASS_UI_TEXT_FIELD && className != EXCEPTION_CLASS_UI_LIST) //TODO: fix internal staticText for Win\Mac
        {
            control->RemoveAllControls();
        }

        if (controlName.IsValid())
        {
            control->SetName(controlName);
        }
    }
    else
    {
        DVASSERT(false);
    }

    controlsStack.push_back(std::make_unique<ControlDescr>(control.Get(), true));

    return ReflectedTypeDB::GetByPointer(control.Get());
}

const ReflectedType* DefaultUIPackageBuilder::BeginControlWithCustomClass(const FastName& controlName, const String& customClassName, const String& className)
{
    RefPtr<UIControl> control(CreateControlByName(customClassName, className));
    DVASSERT(control.Valid());

    if (control.Valid())
    {
        if (className != EXCEPTION_CLASS_UI_TEXT_FIELD && className != EXCEPTION_CLASS_UI_LIST) //TODO: fix internal staticText for Win\Mac
        {
            control->RemoveAllControls();
        }

        if (controlName.IsValid())
        {
            control->SetName(controlName);
        }
    }

    controlsStack.push_back(std::make_unique<ControlDescr>(control.Get(), true));

    return ReflectedTypeDB::GetByPointer(control.Get());
}

const ReflectedType* DefaultUIPackageBuilder::BeginControlWithPrototype(const FastName& controlName, const String& packageName, const FastName& prototypeName, const String* customClassName, AbstractUIPackageLoader* loader)
{
    UIControl* prototype = nullptr;

    if (packageName.empty())
    {
        prototype = package->GetPrototype(prototypeName);
        if (!prototype)
        {
            if (loader->LoadControlByName(prototypeName, this))
                prototype = package->GetPrototype(prototypeName);
        }
    }
    else
    {
        UIPackage* importedPackage = FindImportedPackageByName(packageName);
        if (importedPackage)
            prototype = importedPackage->GetPrototype(prototypeName);
    }

    DVASSERT(prototype != nullptr);

    RefPtr<UIControl> control;
    if (customClassName)
    {
        control = CreateControlByName(*customClassName, "UIControl");

        control->RemoveAllControls();
        control->CopyDataFrom(prototype);
    }
    else
    {
        control.Set(prototype->Clone());
    }

    if (controlName.IsValid())
    {
        control->SetName(controlName);
    }

    control->SetPackageContext(RefPtr<UIControlPackageContext>());

    controlsStack.push_back(std::make_unique<ControlDescr>(control.Get(), true));
    return ReflectedTypeDB::GetByPointer(control.Get());
}

const ReflectedType* DefaultUIPackageBuilder::BeginControlWithPath(const String& pathName)
{
    UIControl* control = nullptr;
    if (!controlsStack.empty())
    {
        control = controlsStack.back()->control->FindByPath(pathName);
    }

    DVASSERT(control);
    controlsStack.push_back(std::make_unique<ControlDescr>(control, false));

    return ReflectedTypeDB::GetByPointer(control);
}

const ReflectedType* DefaultUIPackageBuilder::BeginUnknownControl(const FastName& controlName, const YamlNode* node)
{
    DVASSERT(false);
    controlsStack.push_back(std::make_unique<ControlDescr>(nullptr, false));
    return nullptr;
}

void DefaultUIPackageBuilder::EndControl(eControlPlace controlPlace)
{
    std::unique_ptr<ControlDescr> lastDescr = std::move(controlsStack.back());
    controlsStack.pop_back();

    DVASSERT(lastDescr->control != nullptr);
    lastDescr->control->LoadFromYamlNodeCompleted();

    if (lastDescr->addToParent)
    {
        switch (controlPlace)
        {
        case TO_PROTOTYPES:
        {
            UIControl* control = lastDescr->control.Get();
            GetEngineContext()->uiControlSystem->GetLayoutSystem()->ManualApplyLayout(control);
            package->AddPrototype(control);
            break;
        }

        case TO_CONTROLS:
        {
            UIControl* control = lastDescr->control.Get();
            GetEngineContext()->uiControlSystem->GetLayoutSystem()->ManualApplyLayout(control);
            package->AddControl(control);
            break;
        }

        case TO_PREVIOUS_CONTROL:
        {
            DVASSERT(!controlsStack.empty());
            UIControl* control = controlsStack.back()->control.Get();
            control->AddControl(lastDescr->control.Get());
            break;
        }

        default:
            DVASSERT(false);
            break;
        }
    }
}

void DefaultUIPackageBuilder::BeginControlPropertiesSection(const String& name)
{
    DVASSERT(currentComponentType == nullptr);
    currentObject = ReflectedObject(controlsStack.back()->control.Get());
}

void DefaultUIPackageBuilder::EndControlPropertiesSection()
{
    currentObject = ReflectedObject();
}

const ReflectedType* DefaultUIPackageBuilder::BeginComponentPropertiesSection(const Type* componentType, uint32 componentIndex)
{
    UIControl* control = controlsStack.back()->control.Get();
    UIComponent* component = control->GetComponent(componentType, componentIndex);
    if (component == nullptr)
    {
        component = UIComponent::CreateByType(componentType);
        control->AddComponent(component);
        component->Release();
    }

    currentObject = ReflectedObject(component);
    currentComponentType = componentType;
    return ReflectedTypeDB::GetByPointer(component);
}

void DefaultUIPackageBuilder::EndComponentPropertiesSection()
{
    currentComponentType = nullptr;
    currentObject = ReflectedObject();
}

void DefaultUIPackageBuilder::ProcessProperty(const ReflectedStructure::Field& field, const Any& value)
{
    DVASSERT(currentObject.IsValid());

    if (currentObject.IsValid() && !value.IsEmpty())
    {
        FastName name(field.name);
        int32 propertyIndex = UIStyleSheetPropertyDataBase::Instance()->FindStyleSheetProperty(currentComponentType, name);
        if (propertyIndex >= 0)
        {
            UIControl* control = controlsStack.back()->control.Get();
            control->SetPropertyLocalFlag(propertyIndex, true);
        }

        if (name == PROPERTY_NAME_TEXT)
        {
            field.valueWrapper->SetValueWithCast(currentObject, Any(LocalizedUtf8String(value.Cast<String>())));
        }
        else
        {
            field.valueWrapper->SetValueWithCast(currentObject, value);
        }
    }
}

void DefaultUIPackageBuilder::ProcessDataBinding(const DAVA::String& fieldName, const DAVA::String& expression, DAVA::int32 bindingMode)
{
    UIControl* control = controlsStack.back()->control.Get();
    RefPtr<UIDataBindingComponent> component = MakeRef<UIDataBindingComponent>();
    component->SetControlFieldName(fieldName);
    component->SetBindingExpression(expression);
    component->SetUpdateMode(static_cast<UIDataBindingComponent::UpdateMode>(bindingMode));
    control->AddComponent(component.Get());
}

void DefaultUIPackageBuilder::SetEditorMode(bool editorMode_)
{
    editorMode = editorMode_;
}

void DefaultUIPackageBuilder::PutImportredPackage(const FilePath& path, const RefPtr<UIPackage>& package)
{
    int32 index = static_cast<int32>(importedPackages.size());
    importedPackages.push_back(package);
    packsByPaths[path] = index;
    packsByNames[path.GetBasename()] = index;
}

UIPackage* DefaultUIPackageBuilder::FindImportedPackageByName(const String& name) const
{
    auto it = packsByNames.find(name);
    if (it != packsByNames.end())
        return importedPackages[it->second].Get();

    return nullptr;
}

RefPtr<UIControl> DefaultUIPackageBuilder::CreateControlByName(const String& customClassName, const String& className)
{
    RefPtr<UIControl> c;
    if (!editorMode || ObjectFactory::Instance()->IsTypeRegistered(customClassName))
    {
        c.Set(ObjectFactory::Instance()->New<UIControl>(customClassName));
        DVASSERT(c.Valid());
    }

    if (!c.Valid())
    {
        c.Set(ObjectFactory::Instance()->New<UIControl>(className));
    }
    return c;
}

std::unique_ptr<DefaultUIPackageBuilder> DefaultUIPackageBuilder::CreateBuilder(UIPackagesCache* packagesCache)
{
    std::unique_ptr<DefaultUIPackageBuilder> builder = std::make_unique<DefaultUIPackageBuilder>(packagesCache);
    builder->SetEditorMode(editorMode);
    return builder;
}
}

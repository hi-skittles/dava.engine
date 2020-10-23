#include "UIPackage.h"

#include "UI/UIControl.h"
#include "UI/UIControlPackageContext.h"

namespace DAVA
{
UIPackage::UIPackage()
    : controlPackageContext(MakeRef<UIControlPackageContext>())
{
}

UIPackage::~UIPackage()
{
    controls.clear();

    prototypes.clear();

    controlPackageContext = nullptr;
}

const Vector<RefPtr<UIControl>>& UIPackage::GetPrototypes() const
{
    return prototypes;
}

UIControl* UIPackage::GetPrototype(const String& name) const
{
    return GetPrototype(FastName(name));
}

UIControl* UIPackage::GetPrototype(const FastName& name) const
{
    for (const auto& prototype : prototypes)
    {
        if (prototype->GetName() == name)
            return prototype.Get();
    }

    for (const auto& control : controls) // temporary code for supporting old yaml files
    {
        if (control->GetName() == name)
            return control.Get();
    }

    return nullptr;
}

RefPtr<UIControl> UIPackage::ExtractPrototype(const String& name)
{
    return ExtractPrototype(FastName(name));
}

RefPtr<UIControl> UIPackage::ExtractPrototype(const FastName& name)
{
    RefPtr<UIControl> prototype;
    prototype = GetPrototype(name);
    RemovePrototype(prototype.Get());
    return prototype;
}

void UIPackage::AddPrototype(UIControl* control)
{
    control->SetPackageContext(controlPackageContext);
    prototypes.push_back(RefPtr<UIControl>::ConstructWithRetain(control));
}

void UIPackage::RemovePrototype(UIControl* prototype)
{
    auto iter = std::find(prototypes.begin(), prototypes.end(), RefPtr<UIControl>::ConstructWithRetain(prototype));
    if (iter != prototypes.end())
    {
        prototypes.erase(iter);
    }
}

const Vector<RefPtr<UIControl>>& UIPackage::GetControls() const
{
    return controls;
}

UIControl* UIPackage::GetControl(const String& name) const
{
    return GetControl(FastName(name));
}

UIControl* UIPackage::GetControl(const FastName& name) const
{
    for (const auto& control : controls)
    {
        if (control->GetName() == name)
            return control.Get();
    }

    for (const auto& prototype : prototypes) // temporary code for supporting old yaml files
    {
        if (prototype->GetName() == name)
            return prototype.Get();
    }

    return nullptr;
}

RefPtr<UIControl> UIPackage::ExtractControl(const String& name)
{
    return ExtractControl(FastName(name));
}

RefPtr<UIControl> UIPackage::ExtractControl(const FastName& name)
{
    RefPtr<UIControl> control;
    control = GetControl(name);
    RemoveControl(control.Get());
    return control;
}

void UIPackage::AddControl(UIControl* control)
{
    control->SetPackageContext(controlPackageContext);
    controls.push_back(RefPtr<UIControl>::ConstructWithRetain(control));
}

void UIPackage::RemoveControl(UIControl* control)
{
    auto iter = std::find(controls.begin(), controls.end(), RefPtr<UIControl>::ConstructWithRetain(control));
    if (iter != controls.end())
    {
        controls.erase(iter);
    }
}

UIControlPackageContext* UIPackage::GetControlPackageContext()
{
    return controlPackageContext.Get();
}
}

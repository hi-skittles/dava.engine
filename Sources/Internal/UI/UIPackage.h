#pragma once

#include "Base/BaseObject.h"
#include "Base/RefPtr.h"
#include "Base/RefPtrUtils.h"

namespace DAVA
{
class UIControl;
class UIStyleSheet;
class UIControlPackageContext;

class UIPackage : public BaseObject
{
public:
    static const int32 CURRENT_VERSION = 22;

    UIPackage();

    const Vector<RefPtr<UIControl>>& GetPrototypes() const;
    UIControl* GetPrototype(const String& name) const;
    UIControl* GetPrototype(const FastName& name) const;
    RefPtr<UIControl> ExtractPrototype(const String& name);
    RefPtr<UIControl> ExtractPrototype(const FastName& name);

    void AddPrototype(UIControl* prototype);
    void RemovePrototype(UIControl* control);

    const Vector<RefPtr<UIControl>>& GetControls() const;
    UIControl* GetControl(const String& name) const;
    UIControl* GetControl(const FastName& name) const;
    RefPtr<UIControl> ExtractControl(const String& name);
    RefPtr<UIControl> ExtractControl(const FastName& name);

    void AddControl(UIControl* control);
    void RemoveControl(UIControl* control);

    UIControlPackageContext* GetControlPackageContext();

    template <class C>
    C GetControl(const String& name) const
    {
        return DynamicTypeCheck<C>(GetControl(name));
    }

    template <class C>
    C GetControl(const FastName& name) const
    {
        return DynamicTypeCheck<C>(GetControl(name));
    }

    template <class C>
    C GetPrototype(const String& name) const
    {
        return DynamicTypeCheck<C>(GetPrototype(name));
    }

    template <class C>
    C GetPrototype(const FastName& name) const
    {
        return DynamicTypeCheck<C>(GetPrototype(name));
    }

    template <class C>
    RefPtr<C> ExtractControl(const String& name)
    {
        return ExtractControl<C>(FastName(name));
    }

    template <class C>
    RefPtr<C> ExtractControl(const FastName& name)
    {
        return DynamicTypeCheckRef<C>(ExtractControl(name));
    }

    template <class C>
    RefPtr<C> ExtractPrototype(const String& name)
    {
        return ExtractPrototype<C>(FastName(name));
    }

    template <class C>
    RefPtr<C> ExtractPrototype(const FastName& name)
    {
        return DynamicTypeCheckRef<C>(ExtractPrototype(name));
    }

protected:
    ~UIPackage();

private:
    Vector<RefPtr<UIControl>> prototypes;
    Vector<RefPtr<UIControl>> controls;

    RefPtr<UIControlPackageContext> controlPackageContext;
};
}

#ifndef __DAVAENGINE_UI_PACKAGES_CACHE_H__
#define __DAVAENGINE_UI_PACKAGES_CACHE_H__

#include "Base/BaseObject.h"

namespace DAVA
{
class UIPackage;

class UIPackagesCache final
: public BaseObject
{
public:
    UIPackagesCache(const RefPtr<UIPackagesCache>& _parent);
    UIPackagesCache(UIPackagesCache* _parent = nullptr);

    void PutPackage(const String& name, const RefPtr<UIPackage>& package);
    RefPtr<UIPackage> GetPackage(const String& name) const;

private:
    ~UIPackagesCache() override;

    RefPtr<UIPackagesCache> parent;
    Map<String, RefPtr<UIPackage>> packages;
};
};
#endif // __DAVAENGINE_UI_PACKAGES_CACHE_H__

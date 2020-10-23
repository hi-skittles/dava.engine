#include "UIPackagesCache.h"

#include "UIPackage.h"

namespace DAVA
{
UIPackagesCache::UIPackagesCache(const RefPtr<UIPackagesCache>& _parent)
    : parent(_parent)
{
}

UIPackagesCache::UIPackagesCache(UIPackagesCache* _parent)
    : UIPackagesCache(RefPtr<UIPackagesCache>::ConstructWithRetain(_parent))
{
}

UIPackagesCache::~UIPackagesCache()
{
    parent = nullptr;
    packages.clear();
}

void UIPackagesCache::PutPackage(const String& path, const RefPtr<UIPackage>& package)
{
    auto it = packages.find(path);
    if (it == packages.end())
    {
        packages[path] = package;
    }
    else
    {
        DVASSERT(it->second == package);
    }
}

RefPtr<UIPackage> UIPackagesCache::GetPackage(const String& path) const
{
    auto it = packages.find(path);
    if (it != packages.end())
        return it->second;

    if (parent)
        return parent->GetPackage(path);

    return RefPtr<UIPackage>();
}
}

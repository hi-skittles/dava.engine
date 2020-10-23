#pragma once

#include "Base/BaseTypes.h"

namespace DAVA
{
class UIDataBindingDependenciesManager final
{
public:
    static const int32 UNKNOWN_DEPENDENCY = -1;

    UIDataBindingDependenciesManager();
    ~UIDataBindingDependenciesManager();

    int32 MakeDependency(int32 id, const Vector<void*>& data);
    void AddDependencies(int32 id, const Vector<void*>& data);

    void ReleaseDepencency(int32 index);
    void SetDirty(void* data);
    bool IsDirty(int32 index) const;
    void ResetDirties();

private:
    UnorderedMap<int32, bool> dirtyBindings;
    UnorderedMap<void*, Vector<int32>> dirtyMap;
    int32 nextId = 0;
};
}

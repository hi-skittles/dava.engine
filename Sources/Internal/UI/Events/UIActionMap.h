#pragma once

#include "Base/BaseTypes.h"
#include "Base/BaseObject.h"
#include "Math/Vector.h"
#include "Functional/Function.h"

namespace DAVA
{
class UIActionMap final
{
public:
    typedef DAVA::Function<void(const Any& data)> SimpleAction;

    UIActionMap() = default;
    ~UIActionMap() = default;

    void Put(const FastName& name, const SimpleAction& action);
    void Remove(const FastName& name);
    bool Perform(const FastName& name, const Any& data);

private:
    UnorderedMap<FastName, SimpleAction> actions;
};
}

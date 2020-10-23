#pragma once

#include "UI/Properties/CompletionsProvider.h"
#include <Base/String.h>
#include <Base/FastName.h>
#include <Base/Type.h>

#include <Base/FastName.h>

class PackageBaseNode;
class CompletionsProviderForUIReflection : public CompletionsProvider
{
public:
    CompletionsProviderForUIReflection(const DAVA::String& propertyName, const DAVA::String& componentName = DAVA::String());
    ~CompletionsProviderForUIReflection() override;

    QStringList GetCompletions(AbstractProperty* property) override;

private:
    DAVA::FastName propertyName;
    const DAVA::Type* componentType = nullptr;
};

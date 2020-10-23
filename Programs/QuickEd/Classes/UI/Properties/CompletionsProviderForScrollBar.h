#pragma once

#include "UI/Properties/CompletionsProvider.h"

class PackageBaseNode;

class CompletionsProviderForScrollBar : public CompletionsProvider
{
public:
    CompletionsProviderForScrollBar();
    virtual ~CompletionsProviderForScrollBar();

    QStringList GetCompletions(AbstractProperty* property) override;

private:
    void CollectCompletions(QStringList& list, PackageBaseNode* src, PackageBaseNode* node);
};

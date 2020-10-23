#pragma once

#include "UI/Properties/CompletionsProvider.h"

class PredefinedCompletionsProvider : public CompletionsProvider
{
public:
    PredefinedCompletionsProvider(const QStringList& list);
    ~PredefinedCompletionsProvider() override;

    QStringList GetCompletions(AbstractProperty* property) override;

private:
    QStringList completions;
};

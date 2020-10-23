#pragma once

#include <QStringList>

class AbstractProperty;

class CompletionsProvider
{
public:
    CompletionsProvider()
    {
    }
    virtual ~CompletionsProvider()
    {
    }

    virtual QStringList GetCompletions(AbstractProperty* property) = 0;
};

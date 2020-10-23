#include "PredefinedCompletionsProvider.h"

#include "Model/ControlProperties/AbstractProperty.h"
#include "Model/ControlProperties/RootProperty.h"
#include "Model/PackageHierarchy/ControlNode.h"
#include "Model/PackageHierarchy/PackageNode.h"
#include "Model/PackageHierarchy/PackageNode.h"

#include "UI/UIControl.h"
#include "UI/UIControlHelpers.h"
#include "UI/UIScrollBar.h"

using namespace DAVA;

PredefinedCompletionsProvider::PredefinedCompletionsProvider(const QStringList& list)
    : completions(list)
{
}

PredefinedCompletionsProvider::~PredefinedCompletionsProvider()
{
}

QStringList PredefinedCompletionsProvider::GetCompletions(AbstractProperty* property)
{
    return completions;
}

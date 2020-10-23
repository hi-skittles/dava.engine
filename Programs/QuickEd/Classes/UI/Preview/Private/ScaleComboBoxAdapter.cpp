#include "UI/Preview/ScaleComboBoxAdapter.h"

#include "Modules/CanvasModule/CanvasData.h"

#include <TArc/Core/ContextAccessor.h>

DAVA::FastName ScaleComboBoxAdapter::scalePropertyName{ "scale" };
DAVA::FastName ScaleComboBoxAdapter::enumeratorPropertyName{ "enumerator" };
DAVA::FastName ScaleComboBoxAdapter::enabledPropertyName{ "enabled" };

DAVA_REFLECTION_IMPL(ScaleComboBoxAdapter)
{
    DAVA::ReflectionRegistrator<ScaleComboBoxAdapter>::Begin()
    .Field(scalePropertyName.c_str(), &ScaleComboBoxAdapter::GetScale, &ScaleComboBoxAdapter::SetScale)
    .Field(enumeratorPropertyName.c_str(), &ScaleComboBoxAdapter::GetScales, nullptr)
    .Field(enabledPropertyName.c_str(), &ScaleComboBoxAdapter::IsEnabled, nullptr)
    .End();
}

ScaleComboBoxAdapter::ScaleComboBoxAdapter(DAVA::ContextAccessor* accessor_)
    : accessor(accessor_)
    , canvasDataAdapter(accessor_)
{
}

DAVA::Any ScaleComboBoxAdapter::GetScale() const
{
    if (accessor->GetActiveContext() != nullptr)
    {
        return canvasDataAdapter.GetScale();
    }
    else
    {
        return DAVA::Any();
    }
}

void ScaleComboBoxAdapter::SetScale(const DAVA::Any& scale)
{
    canvasDataAdapter.SetScale(scale.Cast<DAVA::float32>(1.0f));
}

const DAVA::Vector<DAVA::float32>& ScaleComboBoxAdapter::GetScales() const
{
    DAVA::DataContext* activeContext = accessor->GetActiveContext();
    if (activeContext == nullptr)
    {
        static DAVA::Vector<DAVA::float32> emptyData;
        return emptyData;
    }
    else
    {
        CanvasData* canvasData = activeContext->GetData<CanvasData>();
        return canvasData->GetPredefinedScales();
    }
}

bool ScaleComboBoxAdapter::IsEnabled() const
{
    return accessor->GetActiveContext() != nullptr;
}

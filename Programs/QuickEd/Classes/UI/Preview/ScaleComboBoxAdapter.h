#pragma once

#include "Modules/CanvasModule/CanvasDataAdapter.h"

#include <TArc/DataProcessing/DataWrapper.h>

#include <Base/BaseTypes.h>
#include <Reflection/Reflection.h>

namespace DAVA
{
class ContextAccessor;
}

class ScaleComboBoxAdapter
{
public:
    static DAVA::FastName scalePropertyName;
    static DAVA::FastName enumeratorPropertyName;
    static DAVA::FastName enabledPropertyName;

    ScaleComboBoxAdapter(DAVA::ContextAccessor* accessor);

private:
    DAVA::Any GetScale() const;
    void SetScale(const DAVA::Any& scale);

    const DAVA::Vector<DAVA::float32>& GetScales() const;

    bool IsEnabled() const;

    CanvasDataAdapter canvasDataAdapter;
    DAVA::ContextAccessor* accessor = nullptr;

    DAVA_REFLECTION(CanvasData);
};

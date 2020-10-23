#pragma once

#include "Base/BaseTypes.h"
#include "Reflection/ReflectionRegistrator.h"

namespace DAVA
{
struct LayoutMargins final
{
    LayoutMargins();
    LayoutMargins(const LayoutMargins& src);
    LayoutMargins(float32 left, float32 top, float32 right, float32 bottom);

    float32 left = 0.f;
    float32 top = 0.f;
    float32 right = 0.f;
    float32 bottom = 0.f;

private:
    DAVA_REFLECTION(LayoutMargins);
};
}

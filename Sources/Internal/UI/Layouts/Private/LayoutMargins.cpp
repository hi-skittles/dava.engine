#include "UI/Layouts/Private/LayoutMargins.h"
#include "Reflection/ReflectionRegistrator.h"

namespace DAVA
{
DAVA_REFLECTION_IMPL(LayoutMargins)
{
    ReflectionRegistrator<LayoutMargins>::Begin()
    .ConstructorByValue()
    .ConstructorByPointer()
    .DestructorByPointer([](LayoutMargins* m) { SafeDelete(m); })
    .Field("left", &LayoutMargins::left)
    .Field("top", &LayoutMargins::top)
    .Field("right", &LayoutMargins::right)
    .Field("bottom", &LayoutMargins::bottom)
    .End();
}

LayoutMargins::LayoutMargins() = default;

LayoutMargins::LayoutMargins(const LayoutMargins& src)
    : left(src.left)
    , top(src.top)
    , right(src.right)
    , bottom(src.bottom)
{
}

LayoutMargins::LayoutMargins(float32 left, float32 top, float32 right, float32 bottom)
    : left(left)
    , top(top)
    , right(right)
    , bottom(bottom)
{
}
}

#include "HeightmapProxy.h"

HeightmapProxy::HeightmapProxy(Heightmap* heightmap)
    : EditorHeightmap(heightmap)
{
}

void HeightmapProxy::UpdateRect(const DAVA::Rect& rect)
{
    DAVA::int32 size = Size();

    DAVA::Rect bounds(0.f, 0.f, static_cast<DAVA::float32>(size), static_cast<DAVA::float32>(size));

    changedRect = rect;
    bounds.ClampToRect(changedRect);

    heightmapChanged = true;
}

void HeightmapProxy::ResetHeightmapChanged()
{
    heightmapChanged = false;
}

bool HeightmapProxy::IsHeightmapChanged() const
{
    return heightmapChanged;
}

const DAVA::Rect& HeightmapProxy::GetChangedRect() const
{
    return changedRect;
}

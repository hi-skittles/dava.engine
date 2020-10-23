#pragma once

#include "Base/BaseTypes.h"
#include "Base/FastName.h"
#include "Reflection/Reflection.h"
#include "Render/Highlevel/RenderBatch.h"

namespace DAVA
{
class RenderBatchArray : public InspBase
{
public:
    enum
    {
        SORT_ENABLED = 1 << 0,
        SORT_BY_MATERIAL = 1 << 1,
        SORT_BY_DISTANCE_BACK_TO_FRONT = 1 << 2,
        SORT_BY_DISTANCE_FRONT_TO_BACK = 1 << 3,

        SORT_REQUIRED = 1 << 4,
    };

    static const uint32 SORT_THIS_FRAME = SORT_ENABLED | SORT_REQUIRED;

    RenderBatchArray();

    inline void Clear();
    inline void AddRenderBatch(RenderBatch* batch);
    inline uint32 GetRenderBatchCount() const;
    inline RenderBatch* Get(uint32 index) const;

    void Sort(Camera* camera);
    inline void SetSortingFlags(uint32 flags);

private:
    Vector<RenderBatch*> renderBatchArray;
    uint32 sortFlags;
    static bool MaterialCompareFunction(const RenderBatch* a, const RenderBatch* b);
};

inline void RenderBatchArray::Clear()
{
    renderBatchArray.clear();
}

inline void RenderBatchArray::AddRenderBatch(RenderBatch* batch)
{
    renderBatchArray.push_back(batch);
}

inline void RenderBatchArray::SetSortingFlags(uint32 _flags)
{
    sortFlags = _flags;
}

inline uint32 RenderBatchArray::GetRenderBatchCount() const
{
    return uint32(renderBatchArray.size());
}

inline RenderBatch* RenderBatchArray::Get(uint32 index) const
{
    return renderBatchArray[index];
}
}

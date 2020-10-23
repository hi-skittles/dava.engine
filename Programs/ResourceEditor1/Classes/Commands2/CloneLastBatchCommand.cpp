#include "CloneLastBatchCommand.h"
#include "Commands2/RECommandIDs.h"

CloneLastBatchCommand::CloneLastBatchCommand(DAVA::RenderObject* ro)
    : RECommand(CMDID_CLONE_LAST_BATCH, "Clone Last Batch")
{
    DVASSERT(ro);
    renderObject = SafeRetain(ro);

    // find proper LOD and switch indexes and last batches
    maxLodIndexes[0] = maxLodIndexes[1] = -1;
    DAVA::RenderBatch* lastBatches[2] = { NULL, NULL };

    const DAVA::uint32 count = renderObject->GetRenderBatchCount();
    for (DAVA::uint32 i = 0; i < count; ++i)
    {
        DAVA::int32 lod, sw;
        DAVA::RenderBatch* batch = renderObject->GetRenderBatch(i, lod, sw);

        DVASSERT(sw < 2);
        if ((lod > maxLodIndexes[sw]) && (sw >= 0 && sw < 2))
        {
            maxLodIndexes[sw] = lod;
            lastBatches[sw] = batch;
        }
    }
    DVASSERT(maxLodIndexes[0] != maxLodIndexes[1]);

    //detect switch index to clone batches
    requestedSwitchIndex = 0;
    DAVA::int32 maxIndex = maxLodIndexes[1];
    if (maxLodIndexes[0] > maxLodIndexes[1])
    {
        requestedSwitchIndex = 1;
        maxIndex = maxLodIndexes[0];
    }

    //clone batches
    for (DAVA::int32 i = maxLodIndexes[requestedSwitchIndex]; i < maxIndex; ++i)
    {
        newBatches.push_back(lastBatches[requestedSwitchIndex]->Clone());
    }
}

CloneLastBatchCommand::~CloneLastBatchCommand()
{
    SafeRelease(renderObject);

    for_each(newBatches.begin(), newBatches.end(), DAVA::SafeRelease<DAVA::RenderBatch>);
    newBatches.clear();
}

void CloneLastBatchCommand::Redo()
{
    const DAVA::uint32 count = (DAVA::uint32)newBatches.size();
    DAVA::int32 lodIndex = maxLodIndexes[requestedSwitchIndex] + 1;
    for (DAVA::uint32 i = 0; i < count; ++i)
    {
        renderObject->AddRenderBatch(newBatches[i], lodIndex, requestedSwitchIndex);
        ++lodIndex;
    }
}

void CloneLastBatchCommand::Undo()
{
    const DAVA::int32 count = (DAVA::int32)renderObject->GetRenderBatchCount();
    for (DAVA::int32 i = count - 1; i >= 0; --i)
    {
        DAVA::int32 lod, sw;
        renderObject->GetRenderBatch(i, lod, sw);

        if ((sw == requestedSwitchIndex) && (lod > maxLodIndexes[requestedSwitchIndex]))
        {
            renderObject->RemoveRenderBatch(i);
        }
    }
}

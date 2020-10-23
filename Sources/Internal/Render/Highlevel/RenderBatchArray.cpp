#include "Render/Highlevel/RenderBatchArray.h"
#include "Render/Highlevel/RenderSystem.h"
#include "Render/Highlevel/RenderPass.h"

namespace DAVA
{
RenderBatchArray::RenderBatchArray()
    : sortFlags(0)
{
    //sortFlags = SORT_ENABLED | SORT_BY_MATERIAL | SORT_BY_DISTANCE;
    //renderBatchArray.reserve(4096);
}

bool RenderBatchArray::MaterialCompareFunction(const RenderBatch* a, const RenderBatch* b)
{
    return a->layerSortingKey > b->layerSortingKey;
}

void RenderBatchArray::Sort(Camera* camera)
{
    // Need sort
    sortFlags |= SORT_REQUIRED;

    if ((sortFlags & SORT_THIS_FRAME) == SORT_THIS_FRAME)
    {
        if (sortFlags & SORT_BY_MATERIAL)
        {
            //Vector3 cameraPosition = camera->GetPosition();

            for (RenderBatch* batch : renderBatchArray)
            {
                //pointer_size renderObjectId = (pointer_size)batch->GetRenderObject();
                //RenderObject * renderObject = batch->GetRenderObject();
                //Vector3 position = renderObject->GetWorldBoundingBox().GetCenter();
                //float32 distance = (position - cameraPosition).Length();
                //uint32 distanceBits = (0xFFFF - ((uint32)distance) & 0xFFFF);
                uint32 materialIndex = batch->GetMaterial()->GetSortingKey();
                //VI: sorting key has the following layout: (m:8)(s:4)(d:20)
                //batch->layerSortingKey = (pointer_size)((materialIndex << 20) | (batch->GetSortingKey() << 28) | (distanceBits));
                batch->layerSortingKey = static_cast<pointer_size>((materialIndex & 0x0FFFFFFF) | (batch->GetSortingKey() << 28));
                //batch->layerSortingKey = (pointer_size)((batch->GetMaterial()->GetSortingKey() << 20) | (batch->GetSortingKey() << 28) | (renderObjectId & 0x000FFFFF));
            }

            std::sort(renderBatchArray.begin(), renderBatchArray.end(), MaterialCompareFunction);

            sortFlags &= ~SORT_REQUIRED;
        }
        else if (sortFlags & SORT_BY_DISTANCE_BACK_TO_FRONT)
        {
            Vector3 cameraPosition = camera->GetPosition();
            Vector3 cameraDirection = camera->GetDirection();

            for (RenderBatch* batch : renderBatchArray)
            {
                Vector3 delta = batch->GetRenderObject()->GetWorldMatrixPtr()->GetTranslationVector() - cameraPosition;
                uint32 distance = delta.DotProduct(cameraDirection) < 0 ? 0 : (static_cast<uint32>(delta.Length() * 1000.0f)); //x1000.0f is to prevent resorting of nearby objects (still 26 km range)
                distance = distance + 31 - batch->GetSortingOffset();
                batch->layerSortingKey = (distance & 0x0fffffff) | (batch->GetSortingKey() << 28);
            }

            std::stable_sort(renderBatchArray.begin(), renderBatchArray.end(), MaterialCompareFunction);

            sortFlags |= SORT_REQUIRED;
        }
        else if (sortFlags & SORT_BY_DISTANCE_FRONT_TO_BACK)
        {
            Vector3 cameraPosition = camera->GetPosition();

            for (RenderBatch* batch : renderBatchArray)
            {
                RenderObject* renderObject = batch->GetRenderObject();
                Vector3 position = renderObject->GetWorldBoundingBox().GetCenter();
                uint32 distance = static_cast<uint32>((position - cameraPosition).Length() * 100.0f) + 31 - batch->GetSortingOffset();
                uint32 distanceBits = 0x0fffffff - distance & 0x0fffffff;

                batch->layerSortingKey = distanceBits | (batch->GetSortingKey() << 28);
            }

            std::sort(renderBatchArray.begin(), renderBatchArray.end(), MaterialCompareFunction);

            sortFlags |= SORT_REQUIRED;
        }
    }
}
};

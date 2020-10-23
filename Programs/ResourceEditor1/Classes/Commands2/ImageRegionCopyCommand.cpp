#include "Commands2/ImageRegionCopyCommand.h"
#include "Commands2/RECommandIDs.h"
#include "Render/Image/Image.h"
#include "Render/Image/ImageSystem.h"

ImageRegionCopyCommand::ImageRegionCopyCommand(DAVA::Image* _dst, const DAVA::Vector2& dstPos, DAVA::Image* src, const DAVA::Rect& srcRect, DAVA::FilePath _savePath, DAVA::Image* _orig)
    : RECommand(CMDID_IMAGE_REGION_COPY, "Remove entity")
    , dst(_dst)
    , orig(NULL)
    , copy(NULL)
    , pos(dstPos)
    , savePath(_savePath)
{
    SafeRetain(dst);

    if (NULL != src && NULL != dst)
    {
        if (NULL != _orig)
        {
            DVASSERT(_orig->width == srcRect.dx);
            DVASSERT(_orig->height == srcRect.dy);

            orig = _orig;
        }
        else
        {
            orig = DAVA::Image::CopyImageRegion((const DAVA::Image*)dst, DAVA::Rect(dstPos.x, dstPos.y, srcRect.dx, srcRect.dy));
        }

        copy = DAVA::Image::CopyImageRegion((const DAVA::Image*)src, srcRect);
    }
}

ImageRegionCopyCommand::~ImageRegionCopyCommand()
{
    SafeRelease(dst);
    SafeRelease(copy);
    SafeRelease(orig);
}

void ImageRegionCopyCommand::Undo()
{
    if (NULL != dst && NULL != orig)
    {
        dst->InsertImage(orig, pos, DAVA::Rect(0, 0, (DAVA::float32)orig->width, (DAVA::float32)orig->height));
        if (!savePath.IsEmpty())
        {
            DAVA::ImageSystem::Save(savePath, dst);
        }
    }
}

void ImageRegionCopyCommand::Redo()
{
    if (NULL != dst && NULL != copy)
    {
        dst->InsertImage(copy, pos, DAVA::Rect(0, 0, (DAVA::float32)copy->width, (DAVA::float32)copy->height));
        if (!savePath.IsEmpty())
        {
            DAVA::ImageSystem::Save(savePath, dst);
        }
    }
}

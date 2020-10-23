#ifndef __IMAGE_REGION_COPY_COMMAND_H__
#define __IMAGE_REGION_COPY_COMMAND_H__

#include "Render/Image/Image.h"
#include "Commands2/Base/RECommand.h"

class ImageRegionCopyCommand : public RECommand
{
public:
    ImageRegionCopyCommand(DAVA::Image* dst, const DAVA::Vector2& dstPos, DAVA::Image* src, const DAVA::Rect& srcRect, DAVA::FilePath savePath = DAVA::FilePath(), DAVA::Image* orig = NULL);
    ~ImageRegionCopyCommand();

    void Undo() override;
    void Redo() override;

    DAVA::Image* dst;
    DAVA::Image* orig;
    DAVA::Image* copy;
    DAVA::Vector2 pos;
    DAVA::FilePath savePath;
};

#endif // __IMAGE_REGION_COPY_COMMAND_H__

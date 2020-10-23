#pragma once

#include <Render/RenderBase.h>

namespace DAVA
{
/**
    \ingroup imagesystem
    ImageConverter is a helpre class that provides image compression and decompression functionality
*/

class Image;
class ImageConverter
{
public:
    virtual ~ImageConverter() = default;

    /**
        Initialized class with new implementation of compression functionality
     */
    void SetImplementation(ImageConverter* converter);

    /**
        Return true if compression between formats is available
    */
    virtual bool CanConvert(PixelFormat srcFormat, PixelFormat dstFormat) const;

    /**
        Compress or decompress images and return true if operation was successful
     */
    virtual bool Convert(const Image* srcImage, Image* dstImage) const;

private:
    ImageConverter* implementation = nullptr;
};
}

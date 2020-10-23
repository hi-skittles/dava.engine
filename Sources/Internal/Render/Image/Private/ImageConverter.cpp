#include <Render/Image/ImageConverter.h>

namespace DAVA
{
void ImageConverter::SetImplementation(ImageConverter* converter)
{
    implementation = converter;
}

bool ImageConverter::CanConvert(PixelFormat srcFormat, PixelFormat dstFormat) const
{
    if (implementation != nullptr)
    {
        return implementation->CanConvert(srcFormat, dstFormat);
    }
    return false;
}

bool ImageConverter::Convert(const Image* srcImage, Image* dstImage) const
{
    if (implementation != nullptr)
    {
        return implementation->Convert(srcImage, dstImage);
    }
    return false;
}

} //DAVA

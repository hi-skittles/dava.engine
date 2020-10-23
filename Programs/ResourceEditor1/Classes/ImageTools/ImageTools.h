#ifndef __IMAGE_TOOLS_H__
#define __IMAGE_TOOLS_H__

#include <TextureCompression/TextureConverter.h>

#include <Render/Image/Image.h>
#include <FileSystem/FilePath.h>
#include <Render/TextureDescriptor.h>

#include <QImage>

struct Channels
{
    DAVA::ScopedPtr<DAVA::Image> red;
    DAVA::ScopedPtr<DAVA::Image> green;
    DAVA::ScopedPtr<DAVA::Image> blue;
    DAVA::ScopedPtr<DAVA::Image> alpha;

    Channels(const DAVA::ScopedPtr<DAVA::Image>& red_, const DAVA::ScopedPtr<DAVA::Image>& green_, const DAVA::ScopedPtr<DAVA::Image>& blue_, const DAVA::ScopedPtr<DAVA::Image>& alpha_)
        : red(red_)
        , green(green_)
        , blue(blue_)
        , alpha(alpha_)
    {
    }

    inline bool IsEmpty() const
    {
        return (!red || !green || !blue || !alpha);
    }

    inline bool HasFormat(DAVA::PixelFormat format) const
    {
        return (red->GetPixelFormat() == format &&
                green->GetPixelFormat() == format &&
                blue->GetPixelFormat() == format &&
                alpha->GetPixelFormat() == format);
    }

    inline bool ChannelesResolutionEqual() const
    {
        return (red->width == green->width && red->width == blue->width && red->width == alpha->width) &&
        (red->height == green->height && red->height == blue->height && red->height == alpha->height);
    }
};

namespace ImageTools
{
enum eComponentsRGBA
{
    COLOR_RED = 0,
    COLOR_GREEN,
    COLOR_BLUE,
    COLOR_ALPHA,
};

DAVA::uint32 GetTexturePhysicalSize(const DAVA::TextureDescriptor* descriptor, const DAVA::eGPUFamily forGPU, DAVA::uint32 baseMipMaps = 0);
void ConvertImage(const DAVA::TextureDescriptor* descriptor, const DAVA::eGPUFamily forGPU, DAVA::TextureConverter::eConvertQuality quality);

bool SplitImage(const DAVA::FilePath& pathname);

bool MergeImages(const DAVA::FilePath& folder);

Channels CreateSplittedImages(DAVA::Image* originalImage);

DAVA::Image* CreateMergedImage(const Channels& channes);

void SetChannel(DAVA::Image* image, eComponentsRGBA channel, DAVA::uint8 value);

QImage FromDavaImage(const DAVA::FilePath& pathname);
QImage FromDavaImage(const DAVA::Image* image);
}

#endif // __IMAGE_TOOLS_H__

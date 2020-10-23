#include "Classes/Qt/Tools/HeightDeltaTool/PaintHeightDelta.h"

namespace PaintHeightDelta
{
DAVA::Image* CropHeightmapToPow2(DAVA::Heightmap* srcHeightmap)
{
    DAVA::Image* originalImage = DAVA::Image::CreateFromData(srcHeightmap->Size(), srcHeightmap->Size(), DAVA::FORMAT_A16, (DAVA::uint8*)srcHeightmap->Data());

    DAVA::int32 pow2Size = srcHeightmap->Size();

    if (!DAVA::IsPowerOf2(srcHeightmap->Size()))
    {
        DAVA::EnsurePowerOf2(pow2Size);

        if (pow2Size > srcHeightmap->Size())
        {
            pow2Size = pow2Size >> 1;
        }
    }

    if (pow2Size != srcHeightmap->Size())
    {
        DAVA::Image* croppedImage = DAVA::Image::CopyImageRegion(originalImage, pow2Size, pow2Size);
        SafeRelease(originalImage);
        originalImage = croppedImage;
    }

    return originalImage;
}

DAVA::Image* CreateHeightDeltaImage(DAVA::uint32 width, DAVA::uint32 height)
{
    DAVA::Image* targetImage = DAVA::Image::Create(width, height, DAVA::FORMAT_RGBA8888);
    memset(targetImage->data, 0, targetImage->dataSize);
    return targetImage;
}

void CalculateHeightmapToDeltaImageMapping(DAVA::Image* heightmapImage, DAVA::Image* deltaImage, DAVA::float32& widthPixelRatio, DAVA::float32& heightPixelRatio)
{
    DAVA::float32 heightmapWidth = (DAVA::float32)heightmapImage->width;
    DAVA::float32 heightmapHeight = (DAVA::float32)heightmapImage->height;
    DAVA::float32 deltaImageWidth = (DAVA::float32)deltaImage->width;
    DAVA::float32 deltaImageHeight = (DAVA::float32)deltaImage->height;
    widthPixelRatio = deltaImageWidth / heightmapWidth;
    heightPixelRatio = deltaImageHeight / heightmapHeight;
}

DAVA::float32 SampleHeight(DAVA::uint32 x, DAVA::uint32 y, DAVA::Image* heightmapImage, DAVA::float32 terrainHeight)
{
    DAVA::uint32 pixelSize = sizeof(DAVA::uint16);
    DAVA::uint32 rowStride = pixelSize * heightmapImage->width;
    DAVA::uint16 heightmapValue = *(DAVA::uint16*)(heightmapImage->data + y * rowStride + x * pixelSize);
    return ((DAVA::float32)heightmapValue / (DAVA::float32)DAVA::Heightmap::MAX_VALUE) * terrainHeight;
}

void MarkDeltaRegion(DAVA::uint32 x, DAVA::uint32 y, DAVA::float32 widthPixelRatio, DAVA::float32 heightPixelRatio,
                     const DAVA::Color& markColor, DAVA::Image* deltaImage)
{
    DAVA::int32 regionEndX = (DAVA::int32)(x * widthPixelRatio + (DAVA::uint32)widthPixelRatio);
    DAVA::int32 regionEndY = (DAVA::int32)(y * heightPixelRatio + (DAVA::uint32)heightPixelRatio);
    DAVA::int32 rgbaStride = sizeof(DAVA::uint32);

    DAVA::uint8 r = (DAVA::uint8)(markColor.r * 255.0f);
    DAVA::uint8 g = (DAVA::uint8)(markColor.g * 255.0f);
    DAVA::uint8 b = (DAVA::uint8)(markColor.b * 255.0f);
    DAVA::uint8 a = (DAVA::uint8)(markColor.a * 255.0f);

    for (DAVA::int32 yy = (DAVA::int32)(y * heightPixelRatio); yy < regionEndY; ++yy)
    {
        for (DAVA::int32 xx = (DAVA::int32)(x * widthPixelRatio); xx < regionEndX; ++xx)
        {
            DAVA::uint8* dataPtr = deltaImage->data + yy * deltaImage->width * rgbaStride + xx * rgbaStride;
            dataPtr[0] = r;
            dataPtr[1] = g;
            dataPtr[2] = b;
            dataPtr[3] = a;
        }
    }
}

void PrepareDeltaImage(DAVA::Image* heightmapImage, DAVA::Image* deltaImage, DAVA::float32 terrainHeight, DAVA::float32 heightDelta,
                       const DAVA::Vector<DAVA::Color>& colors)
{
    DAVA::float32 widthPixelRatio = 0.0f;
    DAVA::float32 heightPixelRatio = 0.0f;

    CalculateHeightmapToDeltaImageMapping(heightmapImage, deltaImage, widthPixelRatio, heightPixelRatio);

    DVASSERT(widthPixelRatio > 0.0f);
    DVASSERT(heightPixelRatio > 0.0f);

    DAVA::size_type colorCount = colors.size();

    DVASSERT(colorCount >= 2);

    for (DAVA::int32 y = 0; y < (DAVA::int32)heightmapImage->height; ++y)
    {
        for (DAVA::int32 x = 0; x < (DAVA::int32)heightmapImage->width; ++x)
        {
            DAVA::float32 heightXY = SampleHeight(x, y, heightmapImage, terrainHeight);
            DAVA::float32 heightLeft = (x - 1 >= 0) ? SampleHeight(x - 1, y, heightmapImage, terrainHeight) : heightXY;
            DAVA::float32 heightRight = (x + 1 < (DAVA::int32)heightmapImage->width) ? SampleHeight(x + 1, y, heightmapImage, terrainHeight) : heightXY;
            DAVA::float32 heightTop = (y - 1 >= 0) ? SampleHeight(x, y - 1, heightmapImage, terrainHeight) : heightXY;
            DAVA::float32 heightDown = (y + 1 < (DAVA::int32)heightmapImage->height) ? SampleHeight(x, y + 1, heightmapImage, terrainHeight) : heightXY;

            bool isOverThreshold = (DAVA::Abs(heightXY - heightLeft) > heightDelta) ||
            (DAVA::Abs(heightXY - heightRight) > heightDelta) ||
            (DAVA::Abs(heightXY - heightTop) > heightDelta) ||
            (DAVA::Abs(heightXY - heightDown) > heightDelta);

            if (isOverThreshold)
            {
                DAVA::int32 colorIndex = (x + y) % colorCount;
                MarkDeltaRegion((DAVA::uint32)x, (DAVA::uint32)y, widthPixelRatio, heightPixelRatio, colors[colorIndex], deltaImage);
            }
        }
    }
}

void Execute(const DAVA::FilePath& targetImagePath, DAVA::float32 refDelta, DAVA::Heightmap* srcHeightmap,
             DAVA::uint32 targetImageWidth, DAVA::uint32 targetImageHeight, DAVA::float32 targetTerrainHeight,
             const DAVA::Vector<DAVA::Color>& pixelColors)
{
    DAVA::Image* croppedHeightmap = CropHeightmapToPow2(srcHeightmap);
    DAVA::Image* imageDelta = CreateHeightDeltaImage(targetImageWidth, targetImageHeight);
    PrepareDeltaImage(croppedHeightmap, imageDelta, targetTerrainHeight, refDelta, pixelColors);
    DAVA::ImageSystem::Save(targetImagePath, imageDelta);

    SafeRelease(croppedHeightmap);
    SafeRelease(imageDelta);
}
}

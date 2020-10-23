#pragma once

#include "Base/BaseTypes.h"
#include "Base/BaseObject.h"
#include "Base/BaseMath.h"
#include "Render/RenderBase.h"
#include "Utils/Utils.h"

#include "MemoryManager/MemoryProfiler.h"

namespace DAVA
{
class File;
#ifdef __DAVAENGINE_IPHONE__

class SaveToSystemPhotoCallbackReceiver
{
public:
    virtual void SaveToSystemPhotosFinished() = 0;
};

#endif

struct ImageInfo
{
    bool IsEmpty() const
    {
        return (0 == width || 0 == height);
    }

    Size2i GetImageSize() const
    {
        return Size2i(width, height);
    }

    uint32 width = 0;
    uint32 height = 0;
    PixelFormat format = PixelFormat::FORMAT_INVALID;
    uint32 dataSize = 0;
    uint32 mipmapsCount = 0;
    uint32 faceCount = 0;
};

namespace ImageUtils
{
/**
return size of image with given `width`, `height`, `format`
*/
uint32 GetSizeInBytes(uint32 width, uint32 height, PixelFormat format);

/**
returns length in bytes of image line.
pitch size consists of space used by line pixels plus possible additional space used for memory alignment for compressed formats
*/
uint32 GetPitchInBytes(uint32 width, PixelFormat format);
}

class Image : public BaseObject
{
    DAVA_ENABLE_CLASS_ALLOCATION_TRACKING(ALLOC_POOL_IMAGE)

protected:
    virtual ~Image();

public:
    Image();

    /**
    creates new image with given `width`, `height`, `format`
    image data is not initialized
    */
    static Image* Create(uint32 width, uint32 height, PixelFormat format);

    /**
    creates new image with given `width`, `height`, `format`
    image data is initialized with given `data`
    */
    static Image* CreateFromData(uint32 width, uint32 height, PixelFormat format, const uint8* data);

    /**
    creates new image by cloning current image
    */
    Image* Clone() const;

    static Image* CreatePinkPlaceholder(bool checkers = true);
    void MakePink(bool checkers = true);

    inline uint32 GetWidth() const;
    inline uint32 GetHeight() const;
    inline uint8* GetData() const;
    inline uint32 GetDataSize() const;
    inline PixelFormat GetPixelFormat() const;

#ifdef __DAVAENGINE_IPHONE__
    void* GetUIImage();
#endif

    bool Save(const FilePath& path) const;

#ifdef __DAVAENGINE_IPHONE__
    void SaveToSystemPhotos(SaveToSystemPhotoCallbackReceiver* callback = 0);
#endif

    Vector<Image*> CreateMipMapsImages(bool isNormalMap = false);

    bool Normalize();

    // changes size of image canvas to required size, if new size is bigger, sets 0 to all new pixels
    void ResizeCanvas(uint32 newWidth, uint32 newHeight);

    // changes size of image to required size (without any filtration)
    bool ResizeImage(uint32 newWidth, uint32 newHeight);

    static Image* CopyImageRegion(const Image* imageToCopy,
                                  uint32 newWidth, uint32 newHeight,
                                  uint32 xOffset = 0, uint32 yOffset = 0);
    static Image* CopyImageRegion(const Image* imageToCopy, const Rect& rect);

    void InsertImage(const Image* image, uint32 dstX, uint32 dstY,
                     uint32 srcX = 0, uint32 srcY = 0,
                     uint32 srcWidth = -1, uint32 srcHeight = -1);
    void InsertImage(const Image* image, const Vector2& dstPos,
                     const Rect& srcRect);

    // changes size of image canvas to square
    void ResizeToSquare();
    void FlipVertical();
    void FlipHorizontal();
    void RotateDeg(int degree);
    void Rotate90Right();
    void Rotate90Left();

private:
    template <class Type>
    void FlipVertical(Type* buffer, uint32 width, uint32 height);

    template <class Type>
    void FlipHorizontal(Type* buffer, uint32 width, uint32 height);

    template <class Type>
    void Rotate90Right(Type* srcBuffer, Type* dstBuffer, uint32 sideLen);

    template <class Type>
    void Rotate90Left(Type* srcBuffer, Type* dstBuffer, uint32 sideLen);

public:
    uint32 dataSize;
    uint32 width : 16;
    uint32 height : 16;

    uint8* data;

    uint32 mipmapLevel;
    PixelFormat format : 8;

    uint32 cubeFaceID;
    void (*customDeleter)(void*);
};

template <class Type>
void Image::FlipVertical(Type* buffer, uint32 width, uint32 height)
{
    const uint32 halfHeight = (height / 2);
    for (uint32 x = 0; x < width; ++x)
    {
        for (uint32 y = 0; y < halfHeight; ++y)
        {
            Swap(buffer[y * width + x], buffer[(height - 1 - y) * width + x]);
        }
    }
}

template <class Type>
void Image::FlipHorizontal(Type* buffer, uint32 width, uint32 height)
{
    const uint32 halfWidth = width / 2;
    const uint32 maxY = height * width;

    for (uint32 y = 0; y < maxY; y += width)
    {
        for (uint32 x = 0; x < halfWidth; ++x)
        {
            Swap(buffer[y + x], buffer[y + width - x - 1]);
        }
    }
}

template <class Type>
void Image::Rotate90Right(Type* src, Type* dst, uint32 sideLen)
{
    for (uint32 y = 0; y < sideLen; ++y)
    {
        for (uint32 x = 0; x < sideLen; ++x)
        {
            dst[(sideLen - y - 1) + (x * sideLen)] = src[x + y * sideLen];
        }
    }
}

template <class Type>
void Image::Rotate90Left(Type* src, Type* dst, uint32 sideLen)
{
    for (uint32 y = 0; y < sideLen; ++y)
    {
        for (uint32 x = 0; x < sideLen; ++x)
        {
            dst[x + y * sideLen] = src[(sideLen - y - 1) + (x * sideLen)];
        }
    }
}

// Implementation of inline functions
uint32 Image::GetWidth() const
{
    return width;
}

uint32 Image::GetHeight() const
{
    return height;
}

uint8* Image::GetData() const
{
    return data;
}

uint32 Image::GetDataSize() const
{
    return dataSize;
}

PixelFormat Image::GetPixelFormat() const
{
    return format;
}
};

#include "EditorHeightmap.h"
#include "Render/PixelFormatDescriptor.h"

using namespace DAVA;

EditorHeightmap::EditorHeightmap(Heightmap* heightmap)
    : Heightmap()
{
    DVASSERT(heightmap && "Can't be NULL");

    savedHeightmap = SafeRetain(heightmap);
    DownscaleOrClone();

    tableOfChanges = NULL;
    InitializeTableOfChanges();

    scalingTable = NULL;
}

EditorHeightmap::~EditorHeightmap()
{
    SafeDeleteArray(scalingTable);

    SafeDeleteArray(tableOfChanges);
    SafeRelease(savedHeightmap);
}

void EditorHeightmap::DownscaleOrClone()
{
    if (MAX_EDITOR_HEIGHTMAP_SIZE < savedHeightmap->Size())
    {
        Downscale(MAX_EDITOR_HEIGHTMAP_SIZE);
    }
    else
    {
        savedHeightmap->Clone(this);
    }
}

void EditorHeightmap::Downscale(int32 newSize)
{
    ReallocateData(newSize);
    SetTileSize(savedHeightmap->GetTileSize());

    int32 savedHeightmapSize = savedHeightmap->Size() - 1;
    int32 heightmapSide = size - 1;
    int32 multiplier = savedHeightmapSize / heightmapSide;

    for (int32 y = 0; y < heightmapSide; ++y)
    {
        int32 yOffset = y * size;
        for (int32 x = 0; x < heightmapSide; ++x)
        {
            data[yOffset + x] = GetHeightValue(x, y, multiplier);
        }

        data[y * size + (size - 1)] = GetVerticalValue(y, multiplier);
    }

    int32 yOffset = (size - 1) * size;
    for (int32 x = 0; x < heightmapSide; ++x)
    {
        data[yOffset + x] = GetHorizontalValue(x, multiplier);
    }

    data[size * size - 1] = savedHeightmap->Data()[savedHeightmap->Size() * savedHeightmap->Size() - 1]; //left bottom corner
}

void EditorHeightmap::InitializeTableOfChanges()
{
    SafeDeleteArray(tableOfChanges);
    tableOfChanges = new uint8[size * size];
    Memset(tableOfChanges, VALUE_NOT_CHANGED, size * size * sizeof(uint8));
}

uint16 EditorHeightmap::GetHeightValue(int32 posX, int32 posY, int32 muliplier)
{
    uint32 sum = 0;

    int32 firstX = posX * muliplier;
    int32 firstY = posY * muliplier;

    int32 lastX = firstX + muliplier;
    int32 lastY = firstY + muliplier;
    for (int32 y = firstY; y < lastY; ++y)
    {
        int32 yOffset = y * savedHeightmap->Size();
        for (int32 x = firstX; x < lastX; ++x)
        {
            sum += savedHeightmap->Data()[yOffset + x];
        }
    }

    return (uint16)(sum / (muliplier * muliplier));
}

uint16 EditorHeightmap::GetVerticalValue(int32 posY, int32 muliplier)
{
    uint32 sum = 0;

    int32 firstY = posY * muliplier;
    int32 lastY = firstY + muliplier;
    int32 index = firstY * savedHeightmap->Size() + savedHeightmap->Size() - 1;
    for (int32 y = firstY; y < lastY; ++y)
    {
        sum += savedHeightmap->Data()[index];
        index += savedHeightmap->Size();
    }

    return (uint16)(sum / muliplier);
}

uint16 EditorHeightmap::GetHorizontalValue(int32 posX, int32 muliplier)
{
    uint32 sum = 0;

    int32 firstX = posX * muliplier;
    int32 lastX = firstX + muliplier;
    int32 yOffset = (savedHeightmap->Size() - 1) * savedHeightmap->Size();
    for (int32 x = firstX; x < lastX; ++x)
    {
        sum += savedHeightmap->Data()[yOffset + x];
    }

    return (uint16)(sum / muliplier);
}

void EditorHeightmap::HeghtWasChanged(const DAVA::Rect& changedRect)
{
    //    int32 lastY = (int32)(changedRect.y + changedRect.dy);
    //    int32 lastX = (int32)(changedRect.x + changedRect.dx);
    //    for(int32 y = (int32)changedRect.y; y < lastY; ++y)
    //    {
    //        int32 yOffset = y * size;
    //        for(int32 x = (int32)changedRect.x; x < lastX; ++x)
    //        {
    //            tableOfChanges[yOffset + x] = VALUE_WAS_CHANGED;
    //        }
    //    }
}

void EditorHeightmap::Upscale()
{
    int32 savedHeightmapSize = savedHeightmap->Size() - 1;
    int32 heightmapSide = size - 1;
    int32 multiplier = savedHeightmapSize / heightmapSide;

    InitializeScalingTable(multiplier);

    for (int32 y = 0; y < size - 1; ++y)
    {
        int32 yOffset = y * size;
        for (int32 x = 0; x < size - 1; ++x)
        {
            int32 pos = yOffset + x;
            if (VALUE_WAS_CHANGED == tableOfChanges[pos])
            {
                UpscaleValue(x, y, multiplier);
            }
        }

        //last column
        if (VALUE_WAS_CHANGED == tableOfChanges[yOffset + size - 1])
        {
            uint16 top = data[yOffset + size - 1];
            uint16 bottom = data[yOffset + size + size - 1];
            int32 savedOffset = y * multiplier * savedHeightmap->Size() + savedHeightmap->Size() - 1;
            for (int32 i = 0; i < multiplier; ++i)
            {
                savedHeightmap->Data()[savedOffset + i * savedHeightmap->Size()] = (uint16)
                ((float32)top * (1.f - scalingTable[i]) + (float32)bottom * scalingTable[i]);
            }
        }
    }

    //LastRow
    int32 yOffset = (size - 1) * size;
    int32 savedOffset = (savedHeightmap->Size() - 1) * savedHeightmap->Size();
    for (int32 x = 0; x < size - 1; ++x)
    {
        if (VALUE_WAS_CHANGED == tableOfChanges[yOffset + x])
        {
            uint16 left = data[yOffset + x];
            uint16 right = data[yOffset + x + 1];

            int32 offset = savedOffset + x * multiplier;
            for (int32 i = 0; i < multiplier; ++i)
            {
                savedHeightmap->Data()[offset + i] = (uint16)
                ((float32)left * (1.f - scalingTable[i]) + (float32)right * scalingTable[i]);
            }
        }
    }

    if (VALUE_WAS_CHANGED == tableOfChanges[size * size - 1])
    {
        savedHeightmap->Data()[savedHeightmap->Size() * savedHeightmap->Size() - 1] = data[size * size - 1];
    }
}

void EditorHeightmap::InitializeScalingTable(DAVA::int32 count)
{
    DVASSERT((1 < count) && "Wrong count. Must be 2 or greater");

    SafeDeleteArray(scalingTable);
    scalingTable = new float32[count];

    float32 multiplier = 1.f / ((float32)count);
    for (int32 i = 0; i < count; ++i)
    {
        scalingTable[i] = (i + 0) * multiplier;
    }
}

void EditorHeightmap::UpscaleValue(DAVA::int32 leftX, DAVA::int32 topY, DAVA::int32 muliplier)
{
    int32 index = topY * size + leftX;
    uint16 topLeft = data[index];
    uint16 topRight = data[index + 1];
    uint16 bottomLeft = data[index + size];
    uint16 bottomRight = data[index + size + 1];

    int32 startX = leftX * muliplier;
    int32 startY = topY * muliplier;

    for (int32 y = 0; y < muliplier; ++y)
    {
        int32 offset = (startY + y) * savedHeightmap->Size() + startX;

        float32 v_ratio = scalingTable[y];
        float32 v_opposite = 1.f - v_ratio;

        for (int32 x = 0; x < muliplier; ++x)
        {
            float32 u_ratio = scalingTable[x];
            float32 u_opposite = 1.f - u_ratio;

            uint16 value = (uint16)(((float32)topLeft * u_opposite + (float32)topRight * u_ratio) * v_opposite +
                                    ((float32)bottomLeft * u_opposite + (float32)bottomRight * u_ratio) * v_ratio);

            savedHeightmap->Data()[offset + x] = value;
        }
    }
}

bool EditorHeightmap::Clipping(int32& srcOffset,
                               int32& dstOffset,
                               int32& dstX,
                               int32& dstY,
                               int32 dstWidth,
                               int32 dstHeight,
                               int32& width,
                               int32& height,
                               int32& yAddSrc,
                               int32& xAddDst,
                               int32& yAddDst)
{
    int32 xDecLeft = 0;
    int32 xDecRight = 0;
    int32 yDecUp = 0;
    int32 yDecDown = 0;
    int32 nWidth = width;
    int32 nHeight = height;

    int32 clipStartX = 0;
    int32 clipStartY = 0;
    int32 clipEndX = dstWidth - 1;
    int32 clipEndY = dstHeight - 1;

    if (dstX > clipEndX || dstY > clipEndY || dstX + nWidth < clipStartX || dstY + nHeight < clipStartY)
    {
        return false;
    }

    if (dstX < clipStartX)
    {
        xDecLeft = clipStartX - dstX;
    }
    if (dstY < clipStartY)
    {
        yDecUp = clipStartY - dstY;
    }

    if (dstX + nWidth > clipEndX)
    {
        xDecRight = dstX + nWidth - clipEndX - 1;
    }
    if (dstY + nHeight > clipEndY)
    {
        yDecDown = dstY + nHeight - clipEndY - 1;
    }

    width -= (xDecRight + xDecLeft);
    height -= (yDecDown + yDecUp);
    dstOffset = (dstX + dstY * dstWidth) + dstWidth * yDecUp + xDecLeft;
    srcOffset = nWidth * yDecUp + xDecLeft;
    yAddSrc = nWidth - width;
    xAddDst = 1;
    yAddDst = dstWidth - width;

    return true;
}

void EditorHeightmap::DrawRelativeRGBA(Image* src, int32 x, int32 y, int32 width, int32 height, float32 k)
{
    if (src)
    {
        uint16* dstData = Data();
        uint8* srcData = src->data;

        uint32 cntX, cntY;

        int32 yAddSrc;
        int32 xAddDst;
        int32 yAddDst;
        int32 srcOffset;
        int32 dstOffset;

        if (!Clipping(srcOffset, dstOffset, x, y, Size(), Size(), width, height, yAddSrc, xAddDst, yAddDst))
            return;

        srcData += (srcOffset * 4);
        dstData += dstOffset;

        uint8* changes = tableOfChanges + dstOffset;

        yAddSrc *= 4;
        for (cntY = height; cntY > 0; cntY--)
        {
            for (cntX = width; cntX > 0; cntX--)
            {
                float32 newValue = *dstData + (*srcData * k);
                if (newValue < 0.f)
                {
                    newValue = 0.f;
                }
                else if (Heightmap::MAX_VALUE < newValue)
                {
                    newValue = Heightmap::MAX_VALUE;
                }

                if (newValue != *dstData)
                {
                    *dstData = (uint16)newValue;
                    *changes = VALUE_WAS_CHANGED;
                }

                srcData += 4;
                dstData += xAddDst;

                changes += xAddDst;
            }
            srcData += yAddSrc;
            dstData += yAddDst;
            changes += yAddDst;
        }
    }
}

void EditorHeightmap::DrawAverageRGBA(Image* mask, int32 x, int32 y, int32 width, int32 height, float32 k)
{
    if (mask)
    {
        uint16* dstData = Data();
        uint8* maskData = mask->data;

        uint32 cntX, cntY;

        int32 yAddSrc;
        int32 xAddDst;
        int32 yAddDst;
        int32 srcOffset;
        int32 dstOffset;

        if (!Clipping(srcOffset, dstOffset, x, y, Size(), Size(), width, height, yAddSrc, xAddDst, yAddDst))
            return;

        maskData += (srcOffset * 4);
        dstData += dstOffset;

        uint8* changes = tableOfChanges + dstOffset;

        uint16* dstDataSaved = dstData;
        uint8* maskDataSaved = maskData;

        float64 average = 0.f;
        float64 maskMax = 0.f;
        int32 count = 0;

        yAddSrc *= 4;
        for (cntY = height; cntY > 0; cntY--)
        {
            for (cntX = width; cntX > 0; cntX--)
            {
                if (*maskData)
                {
                    maskMax = Max(maskMax, (float64)*maskData);

                    average += *dstData;
                    ++count;
                }

                maskData += 4;
                dstData += xAddDst;
            }
            maskData += yAddSrc;
            dstData += yAddDst;
        }

        if (count && k && maskMax)
        {
            average /= count;

            for (cntY = height; cntY > 0; cntY--)
            {
                for (cntX = width; cntX > 0; cntX--)
                {
                    if (*maskDataSaved)
                    {
                        float64 koef = Min(k * ((float64)*maskDataSaved / maskMax), (float64)1.0f);

                        float64 newValue = (float64)*dstDataSaved + (float64)(average - *dstDataSaved) * koef;
                        if (newValue < 0.f)
                        {
                            newValue = 0.f;
                        }
                        else if (Heightmap::MAX_VALUE < newValue)
                        {
                            newValue = Heightmap::MAX_VALUE;
                        }

                        if (*dstDataSaved != (uint16)newValue)
                        {
                            *dstDataSaved = (uint16)newValue;
                            *changes = VALUE_WAS_CHANGED;
                        }
                    }

                    maskDataSaved += 4;
                    dstDataSaved += xAddDst;
                    changes += xAddDst;
                }
                maskDataSaved += yAddSrc;
                dstDataSaved += yAddDst;
                changes += yAddDst;
            }
        }
    }
}

void EditorHeightmap::DrawAbsoluteRGBA(Image* mask, int32 x, int32 y, int32 width, int32 height, float32 k, float32 dstHeight)
{
    if (mask)
    {
        uint16* dstData = Data();
        uint8* maskData = mask->data;

        uint32 cntX, cntY;

        int32 yAddSrc;
        int32 xAddDst;
        int32 yAddDst;
        int32 srcOffset;
        int32 dstOffset;

        if (!Clipping(srcOffset, dstOffset, x, y, Size(), Size(), width, height, yAddSrc, xAddDst, yAddDst))
            return;

        maskData += (srcOffset * 4);
        dstData += dstOffset;

        uint8* changes = tableOfChanges + dstOffset;

        uint16* dstDataSaved = dstData;
        uint8* maskDataSaved = maskData;

        float64 maskMax = 0.f;

        yAddSrc *= 4;
        for (cntY = height; cntY > 0; cntY--)
        {
            for (cntX = width; cntX > 0; cntX--)
            {
                if (*maskData)
                {
                    maskMax = Max(maskMax, (float64)*maskData);
                }

                maskData += 4;
                dstData += xAddDst;
            }
            maskData += yAddSrc;
            dstData += yAddDst;
        }

        if (k && maskMax)
        {
            for (cntY = height; cntY > 0; cntY--)
            {
                for (cntX = width; cntX > 0; cntX--)
                {
                    if (*maskDataSaved)
                    {
                        float64 newValue = (float64)*dstDataSaved +
                        (float64)(dstHeight - *dstDataSaved) * k * ((float64)*maskDataSaved / maskMax);
                        if (newValue < 0.f)
                        {
                            newValue = 0.f;
                        }
                        else if (Heightmap::MAX_VALUE < newValue)
                        {
                            newValue = Heightmap::MAX_VALUE;
                        }

                        if (*dstDataSaved != (uint16)newValue)
                        {
                            *dstDataSaved = (uint16)newValue;
                            *changes = VALUE_WAS_CHANGED;
                        }
                    }

                    maskDataSaved += 4;
                    dstDataSaved += xAddDst;
                    changes += xAddDst;
                }
                maskDataSaved += yAddSrc;
                dstDataSaved += yAddDst;
                changes += yAddDst;
            }
        }
    }
}

void EditorHeightmap::DrawCopypasteRGBA(Image* mask, const Vector2& posFrom, const Vector2& posTo, int32 width, int32 height, float32 koef)
{
    //Find max mask value
    float64 maskMax = 0;
    uint8* maskData = mask->GetData();
    for (int32 iRow = 0; iRow < height; ++iRow)
    {
        for (int32 iCol = 0; iCol < width; ++iCol)
        {
            maskMax = Max(maskMax, (float64)*maskData);
            maskData += 4;
        }
    }

    if (!maskMax)
        return;

    //copy-paste
    uint16* dstData = Data();
    for (int32 iRow = 0; iRow < height; ++iRow)
    {
        int32 ySrc = (int32)(posFrom.y + iRow);
        int32 yDst = (int32)(posTo.y + iRow);

        if ((0 <= ySrc && ySrc < Size())
            && (0 <= yDst && yDst < Size()))
        {
            int32 srcIndex = ySrc * Size();
            int32 dstIndex = yDst * Size();

            for (int32 iCol = 0; iCol < width; ++iCol)
            {
                int32 xSrc = (int32)(posFrom.x + iCol);
                int32 xDst = (int32)(posTo.x + iCol);

                if ((0 <= xSrc && xSrc < Size())
                    && (0 <= xDst && xDst < Size()))
                {
                    uint8 maskData = mask->data[(iRow * width + iCol) * 4];
                    if (maskData)
                    {
                        int32 dstOffset = (dstIndex + xDst);
                        int32 srcOffset = (srcIndex + xSrc);

                        uint16 newValue = dstData[dstOffset] +
                        (uint16)((dstData[srcOffset] - dstData[dstOffset]) * koef * maskData / maskMax);

                        if (dstData[dstOffset] != newValue)
                        {
                            dstData[dstOffset] = newValue;
                            tableOfChanges[dstOffset] = VALUE_WAS_CHANGED;
                        }
                    }
                }
            }
        }
    }
}

void EditorHeightmap::DrawCopypasteRGBA(Image* src, Image* dst, Image* mask, const Vector2& posFrom, const Vector2& posTo, int32 width, int32 height)
{
    DVASSERT(src->width == dst->width);
    DVASSERT(src->height == dst->height);
    DVASSERT(src->format == dst->format);

    DVASSERT(dst->format == FORMAT_RGBA8888,
             Format("Can't use %s with format %s", __FUNCTION__, PixelFormatDescriptor::GetPixelFormatString(dst->format)).c_str());

    static const int32 formatSizeInBytes = 4;

    //copy-paste
    uint8* srcData = src->data;
    uint8* dstData = dst->data;
    for (int32 iRow = 0; iRow < height; ++iRow)
    {
        uint32 ySrc = (uint32)(posFrom.y + iRow);
        uint32 yDst = (uint32)(posTo.y + iRow);

        if (ySrc < src->height && yDst < dst->height)
        {
            int32 srcIndex = ySrc * src->width;
            int32 dstIndex = yDst * dst->width;

            for (int32 iCol = 0; iCol < width; ++iCol)
            {
                uint32 xSrc = (uint32)(posFrom.x + iCol);
                uint32 xDst = (uint32)(posTo.x + iCol);

                if ((xSrc < src->width) && (xDst < dst->width))
                {
                    // mask size could be not equal to the copy/paste area size
                    uint32 maskY = iRow * mask->GetHeight() / height;
                    uint32 maskX = iCol * mask->GetWidth() / width;
                    uint8 maskData = mask->data[(maskY * mask->width + maskX) * formatSizeInBytes];
                    if (maskData)
                    {
                        int64 dstOffset = (dstIndex + xDst) * formatSizeInBytes;
                        int64 srcOffset = (srcIndex + xSrc) * formatSizeInBytes;

                        for (int32 i = 0; i < formatSizeInBytes; ++i)
                        {
                            dstData[dstOffset + i] = srcData[srcOffset + i];
                        }
                    }
                }
            }
        }
    }
}

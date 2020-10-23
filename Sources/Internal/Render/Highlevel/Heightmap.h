#pragma once

#include "Base/BaseObject.h"
#include "FileSystem/FilePath.h"
#include "Base/BaseMath.h"

namespace DAVA
{
class Image;
class Heightmap : public BaseObject
{
protected:
    virtual ~Heightmap();

public:
    static const int32 MAX_VALUE = 65535;
    static const int32 IMAGE_CORRECTION = MAX_VALUE / 255;

    Heightmap(int32 size = 0);

    bool BuildFromImage(const Image* image);
    void SaveToImage(const FilePath& filename);

    virtual void Save(const FilePath& filePathname);
    virtual bool Load(const FilePath& filePathname);

    uint16 GetHeight(uint16 x, uint16 y) const;
    uint16 GetHeightClamp(uint16 x, uint16 y) const;

    Vector3 GetPoint(uint16 x, uint16 y, const AABBox3& bbox) const;

    int32 Size() const;
    uint16* Data();

    int32 GetTileSize() const;
    void SetTileSize(int32 newSize);

    Heightmap* Clone(Heightmap* clonedHeightmap);

    static const String& FileExtension();

protected:
    void ReallocateData(int32 newSize);

    DAVA_DEPRECATED(void LoadNotPow2(File* file, int32 readMapSize, int32 readTileSize));

    uint16* data = nullptr;
    int32 size = 0;
    int32 tileSize = 16;

    static String FILE_EXTENSION;

    DAVA_VIRTUAL_REFLECTION(Heightmap, BaseObject);
};
inline uint16 Heightmap::GetHeightClamp(uint16 x, uint16 y) const
{
    uint16 hm_1 = uint16(size - 1);
    return data[Min(x, hm_1) + Min(y, hm_1) * size];
}

inline uint16 Heightmap::GetHeight(uint16 x, uint16 y) const
{
    DVASSERT(x < size && y < size);
    return data[x + y * size];
}

inline Vector3 Heightmap::GetPoint(uint16 x, uint16 y, const AABBox3& bbox) const
{
    Vector3 res;
    res.x = (bbox.min.x + x / float32(size) * (bbox.max.x - bbox.min.x));
    res.y = (bbox.min.y + y / float32(size) * (bbox.max.y - bbox.min.y));
    res.z = (bbox.min.z + GetHeightClamp(x, y) / float32(Heightmap::MAX_VALUE) * (bbox.max.z - bbox.min.z));
    return res;
}

inline uint16* Heightmap::Data()
{
    return data;
}

inline int32 Heightmap::Size() const
{
    return size;
}

inline int32 Heightmap::GetTileSize() const
{
    return tileSize;
}

inline void Heightmap::SetTileSize(int32 newSize)
{
    tileSize = newSize;
}

inline const String& Heightmap::FileExtension()
{
    return FILE_EXTENSION;
}
}

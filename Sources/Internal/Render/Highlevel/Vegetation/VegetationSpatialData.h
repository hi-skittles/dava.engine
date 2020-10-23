#ifndef __DAVAENGINE_VEGETATIONSPATIALDATA_H__
#define __DAVAENGINE_VEGETATIONSPATIALDATA_H__

namespace DAVA
{
/**
 \brief Data chunk associated with a vegetation quad tree. 
    This data is used for visibility culling and rendering of tree leafes with proper geometry buffers.
 */
struct VegetationSpatialData
{
    int16 x;
    int16 y;
    int16 width;
    int16 height;
    int8 rdoIndex;
    bool isVisible;

    Vector2 animationOffset[4];
    Vector2 animationVelocity[4];

    AABBox3 bbox;
    float32 cameraDistance;
    uint8 clippingPlane;

    inline VegetationSpatialData();
    inline VegetationSpatialData& operator=(const VegetationSpatialData& src);
    inline static bool IsEmpty(uint32 cellValue);
    inline bool IsRenderable() const;
    inline int16 GetResolutionId() const;
    inline bool IsVisibleInResolution(uint32 resolutionId, uint32 maxResolutions) const;
    inline bool IsElementaryCell() const;
};

inline VegetationSpatialData::VegetationSpatialData()
    : x(-1)
    , y(-1)
    , width(-1)
    , height(-1)
    , rdoIndex(-1)
    , isVisible(true)
    , cameraDistance(0.0f)
    , clippingPlane(0)
{
}

inline VegetationSpatialData& VegetationSpatialData::operator=(const VegetationSpatialData& src)
{
    x = src.x;
    y = src.y;
    bbox = src.bbox;
    cameraDistance = src.cameraDistance;
    clippingPlane = src.clippingPlane;
    width = src.width;
    height = src.height;
    rdoIndex = src.rdoIndex;

    return *this;
}

inline bool VegetationSpatialData::IsEmpty(uint32 cellValue)
{
    return (0 == (cellValue & 0x0F0F0F0F));
}

inline bool VegetationSpatialData::IsVisibleInResolution(uint32 resolutionId, uint32 maxResolutions) const
{
    uint32 refResolution = ((x * y) % maxResolutions);
    return (refResolution >= resolutionId);
}

inline bool VegetationSpatialData::IsRenderable() const
{
    return (width > 0 && height > 0);
}

inline int16 VegetationSpatialData::GetResolutionId() const
{
    return (width * height);
}

bool VegetationSpatialData::IsElementaryCell() const
{
    return (1 == width);
}
};

#endif

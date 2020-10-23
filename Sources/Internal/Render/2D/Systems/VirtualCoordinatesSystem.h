#pragma once

#include "Base/BaseTypes.h"
#include "Base/BaseMath.h"
#include "Functional/Signal.h"

namespace DAVA
{
class VirtualCoordinatesSystem final
{
    struct ResourceSpaceSize
    {
        ResourceSpaceSize()
            : width(0)
            , height(0)
            , toVirtual(0)
            , toPhysical(0)
        {
        }
        int32 width;
        int32 height;
        String folderName;
        float32 toVirtual;
        float32 toPhysical;
    };

public:
    VirtualCoordinatesSystem();
    virtual ~VirtualCoordinatesSystem(){};

    void SetVirtualScreenSize(int32 width, int32 height);
    void SetPhysicalScreenSize(int32 width, int32 height);
    void SetInputScreenAreaSize(int32 width, int32 height);

    inline const Size2i& GetVirtualScreenSize() const;
    inline const Size2i& GetRequestedVirtualScreenSize() const;
    inline const Size2i& GetPhysicalScreenSize() const;
    inline const Size2i& GetInputScreenSize() const;
    inline const Rect& GetFullScreenVirtualRect() const;

    inline float32 AlignVirtualToPhysicalX(float32) const;
    inline float32 AlignVirtualToPhysicalY(float32) const;

    inline float32 ConvertPhysicalToVirtualX(float32) const;
    inline float32 ConvertPhysicalToVirtualY(float32 value) const;
    inline float32 ConvertVirtualToPhysicalX(float32 value) const;
    inline float32 ConvertVirtualToPhysicalY(float32 value) const;
    inline float32 ConvertVirtualToInputX(float32 value) const;
    inline float32 ConvertVirtualToInputY(float32 value) const;

    inline float32 ConvertResourceToVirtualX(float32 value, int32 resourceIndex) const;
    inline float32 ConvertResourceToVirtualY(float32 value, int32 resourceIndex) const;
    inline float32 ConvertVirtualToResourceX(float32 value, int32 resourceIndex) const;
    inline float32 ConvertVirtualToResourceY(float32 value, int32 resourceIndex) const;

    inline Vector2 ConvertPhysicalToVirtual(const Vector2& vector) const;
    inline Vector2 ConvertVirtualToPhysical(const Vector2& vector) const;
    inline Vector2 ConvertResourceToVirtual(const Vector2& vector, int32 resourceIndex) const;
    inline Vector2 ConvertResourceToPhysical(const Vector2& vector, int32 resourceIndex) const;
    inline Vector2 ConvertVirtualToResource(const Vector2& value, int32 resourceIndex) const;
    inline Vector2 ConvertInputToVirtual(const Vector2& vector) const;
    inline Vector2 ConvertVirtualToInput(const Vector2& vector) const;

    inline Rect ConvertPhysicalToVirtual(const Rect& rect) const;
    inline Rect ConvertVirtualToPhysical(const Rect& rect) const;
    inline Rect ConvertResourceToVirtual(const Rect& rect, int32 resourceIndex) const;
    inline Rect ConvertResourceToPhysical(const Rect& rect, int32 resourceIndex) const;
    inline Rect ConvertInputToVirtual(const Rect& rect) const;
    inline Rect ConvertVirtualToInput(const Rect& rect) const;

    DAVA_DEPRECATED(inline uint32 GetResourceFoldersCount() const);
    DAVA_DEPRECATED(inline const String& GetResourceFolder(int32 resourceIndex) const);
    DAVA_DEPRECATED(inline int32 GetDesirableResourceIndex() const);
    DAVA_DEPRECATED(inline void SetDesirableResourceIndex(int32 resourceIndex));
    DAVA_DEPRECATED(inline int32 GetBaseResourceIndex() const);

    inline bool WasScreenSizeChanged() const;
    void ScreenSizeChanged();

    DAVA_DEPRECATED(void EnableReloadResourceOnResize(bool enable));
    DAVA_DEPRECATED(bool GetReloadResourceOnResize() const);

    void SetProportionsIsFixed(bool needFixed);

    DAVA_DEPRECATED(void RegisterAvailableResourceSize(int32 width, int32 height, const String& resourcesFolderName));
    DAVA_DEPRECATED(void UnregisterAllAvailableResourceSizes());

    inline const Vector2& GetPhysicalDrawOffset() const;

    Signal<const Size2i&> physicalSizeChanged;
    Signal<const Size2i&> virtualSizeChanged;
    Signal<const Size2i&> inputAreaSizeChanged;

private:
    inline Rect ConvertRect(const Rect& rect, float32 factor) const;

    Vector<ResourceSpaceSize> allowedSizes;

    Size2i virtualScreenSize;
    Size2i requestedVirtualScreenSize;
    Size2i physicalScreenSize;
    Rect fullVirtualScreenRect;

    float32 virtualToPhysical;
    float32 physicalToVirtual;
    Vector2 drawOffset;

    int32 desirableIndex;
    bool fixedProportions;

    bool wasScreenResized;
    bool enabledReloadResourceOnResize;

    Size2i inputAreaSize;
    float32 inputScaleFactor;
    Vector2 inputOffset;
};

inline const Size2i& VirtualCoordinatesSystem::GetVirtualScreenSize() const
{
    return virtualScreenSize;
}

inline const Size2i& VirtualCoordinatesSystem::GetRequestedVirtualScreenSize() const
{
    return requestedVirtualScreenSize;
}

inline const Size2i& VirtualCoordinatesSystem::GetInputScreenSize() const
{
    return inputAreaSize;
}

inline const Size2i& VirtualCoordinatesSystem::GetPhysicalScreenSize() const
{
    return physicalScreenSize;
}

inline const Rect& VirtualCoordinatesSystem::GetFullScreenVirtualRect() const
{
    return fullVirtualScreenRect;
}

inline float32 VirtualCoordinatesSystem::AlignVirtualToPhysicalX(float32 value) const
{
    return std::floor(value / physicalToVirtual + 0.5f) * physicalToVirtual;
}

inline float32 VirtualCoordinatesSystem::AlignVirtualToPhysicalY(float32 value) const
{
    return std::floor(value / physicalToVirtual + 0.5f) * physicalToVirtual;
}

inline float32 VirtualCoordinatesSystem::ConvertPhysicalToVirtualX(float32 value) const
{
    return physicalToVirtual * value;
}

inline float32 VirtualCoordinatesSystem::ConvertPhysicalToVirtualY(float32 value) const
{
    return physicalToVirtual * value;
}

inline float32 VirtualCoordinatesSystem::ConvertVirtualToPhysicalX(float32 value) const
{
    return virtualToPhysical * value;
}

inline float32 VirtualCoordinatesSystem::ConvertVirtualToPhysicalY(float32 value) const
{
    return virtualToPhysical * value;
}

inline float32 VirtualCoordinatesSystem::ConvertVirtualToInputX(float32 value) const
{
    return value / inputScaleFactor;
}

inline float32 VirtualCoordinatesSystem::ConvertVirtualToInputY(float32 value) const
{
    return value / inputScaleFactor;
}

inline float32 VirtualCoordinatesSystem::ConvertResourceToVirtualX(float32 value, DAVA::int32 resourceIndex) const
{
    DVASSERT(resourceIndex < int32(allowedSizes.size()));
    return value * allowedSizes[resourceIndex].toVirtual;
}

inline float32 VirtualCoordinatesSystem::ConvertResourceToVirtualY(float32 value, DAVA::int32 resourceIndex) const
{
    DVASSERT(resourceIndex < int32(allowedSizes.size()));
    return value * allowedSizes[resourceIndex].toVirtual;
}

inline float32 VirtualCoordinatesSystem::ConvertVirtualToResourceX(float32 value, DAVA::int32 resourceIndex) const
{
    DVASSERT(resourceIndex < int32(allowedSizes.size()));
    return value / allowedSizes[resourceIndex].toVirtual;
}

inline float32 VirtualCoordinatesSystem::ConvertVirtualToResourceY(float32 value, DAVA::int32 resourceIndex) const
{
    DVASSERT(resourceIndex < int32(allowedSizes.size()));
    return value / allowedSizes[resourceIndex].toVirtual;
}

inline Vector2 VirtualCoordinatesSystem::ConvertPhysicalToVirtual(const Vector2& vector) const
{
    return vector * physicalToVirtual;
}

inline Vector2 VirtualCoordinatesSystem::ConvertVirtualToPhysical(const Vector2& vector) const
{
    return vector * virtualToPhysical;
}

inline Vector2 VirtualCoordinatesSystem::ConvertResourceToVirtual(const Vector2& vector, DAVA::int32 resourceIndex) const
{
    DVASSERT(resourceIndex < int32(allowedSizes.size()));
    return vector * allowedSizes[resourceIndex].toVirtual;
}

inline Vector2 VirtualCoordinatesSystem::ConvertResourceToPhysical(const Vector2& vector, DAVA::int32 resourceIndex) const
{
    DVASSERT(resourceIndex < int32(allowedSizes.size()));
    return vector * allowedSizes[resourceIndex].toPhysical;
}

inline Vector2 VirtualCoordinatesSystem::ConvertVirtualToResource(const Vector2& value, int32 resourceIndex) const
{
    DVASSERT(resourceIndex < int32(allowedSizes.size()));
    return value / allowedSizes[resourceIndex].toVirtual;
}

inline Rect VirtualCoordinatesSystem::ConvertPhysicalToVirtual(const Rect& rect) const
{
    return ConvertRect(rect, physicalToVirtual);
}

inline Rect VirtualCoordinatesSystem::ConvertVirtualToPhysical(const Rect& rect) const
{
    return ConvertRect(rect, virtualToPhysical);
}

inline Rect VirtualCoordinatesSystem::ConvertResourceToVirtual(const Rect& rect, int32 resourceIndex) const
{
    DVASSERT(resourceIndex < int32(allowedSizes.size()));
    return ConvertRect(rect, allowedSizes[resourceIndex].toVirtual);
}

inline Rect VirtualCoordinatesSystem::ConvertResourceToPhysical(const Rect& rect, int32 resourceIndex) const
{
    DVASSERT(resourceIndex < int32(allowedSizes.size()));
    return ConvertRect(rect, allowedSizes[resourceIndex].toPhysical);
}

inline Rect VirtualCoordinatesSystem::ConvertRect(const Rect& rect, float32 factor) const
{
    Rect newRect(rect);
    newRect.x *= factor;
    newRect.y *= factor;
    newRect.dx *= factor;
    newRect.dy *= factor;

    return newRect;
}

inline Vector2 VirtualCoordinatesSystem::ConvertVirtualToInput(const Vector2& point) const
{
    Vector2 calcPoint(point);

    calcPoint -= inputOffset;
    calcPoint /= inputScaleFactor;

    return calcPoint;
}

inline Vector2 VirtualCoordinatesSystem::ConvertInputToVirtual(const Vector2& point) const
{
    Vector2 calcPoint(point);

    calcPoint *= inputScaleFactor;
    calcPoint += inputOffset;

    return calcPoint;
}

inline Rect VirtualCoordinatesSystem::ConvertInputToVirtual(const Rect& rect) const
{
    return Rect(ConvertInputToVirtual(rect.GetPosition()), ConvertInputToVirtual(rect.GetSize()) - inputOffset);
}

inline Rect VirtualCoordinatesSystem::ConvertVirtualToInput(const Rect& rect) const
{
    return Rect(ConvertVirtualToInput(rect.GetPosition()), ConvertVirtualToInput(rect.GetSize() + inputOffset));
}

inline uint32 VirtualCoordinatesSystem::GetResourceFoldersCount() const
{
    return static_cast<uint32>(allowedSizes.size());
}

inline const String& VirtualCoordinatesSystem::GetResourceFolder(int32 resourceIndex) const
{
    DVASSERT(resourceIndex < int32(allowedSizes.size()));
    return allowedSizes[resourceIndex].folderName;
}

inline int32 VirtualCoordinatesSystem::GetDesirableResourceIndex() const
{
    return desirableIndex;
}

inline void VirtualCoordinatesSystem::SetDesirableResourceIndex(int32 resourceIndex)
{
    DVASSERT(resourceIndex >= 0 && resourceIndex < int32(allowedSizes.size()));
    desirableIndex = resourceIndex;
}

inline int32 VirtualCoordinatesSystem::GetBaseResourceIndex() const
{
    return 0;
}

inline bool VirtualCoordinatesSystem::WasScreenSizeChanged() const
{
    return wasScreenResized;
}

const Vector2& VirtualCoordinatesSystem::GetPhysicalDrawOffset() const
{
    return drawOffset;
}
};

#pragma once

#include "Base/BaseTypes.h"
#include "Base/BaseMath.h"

namespace DAVA
{
class DynamicBitset
{
public:
    DynamicBitset(uint32 elementCount = 256);
    ~DynamicBitset();

    void Clear();
    void Resize(uint32 size);
    bool At(uint32 index);
    void Set(uint32 index, bool value);

    uint32 GetSize() const;
    pointer_size* GetData() const;

private:
    enum : pointer_size
    {
        SHIFT_BITS = StaticLog2<sizeof(pointer_size) * 8>::value,
        AND_BITS = (1 << SHIFT_BITS) - 1,
    };
    void ReallocateSize(uint32 _newMaxElementCount);

private:
    uint32 elementCount = 0;
    pointer_size* dataArray = 0;
};

inline DynamicBitset::DynamicBitset(uint32 _elementCount)
{
    ReallocateSize(_elementCount);
}

inline DynamicBitset::~DynamicBitset()
{
    SafeDeleteArray(dataArray);
}

inline uint32 DynamicBitset::GetSize() const
{
    return elementCount;
}

inline void DynamicBitset::Clear()
{
    Memset(dataArray, 0, sizeof(pointer_size) * ((elementCount >> SHIFT_BITS) + 1));
}

inline void DynamicBitset::Resize(uint32 _newSize)
{
    ReallocateSize(_newSize);
}

inline void DynamicBitset::Set(uint32 index, bool value)
{
    pointer_size arrayIndex = static_cast<pointer_size>(index) >> SHIFT_BITS;
    pointer_size bitElement = static_cast<pointer_size>(index) & AND_BITS;
    if (value)
        dataArray[arrayIndex] |= (1ull << bitElement);
    else
        dataArray[arrayIndex] &= ~(1ull << bitElement);
}

inline bool DynamicBitset::At(uint32 index)
{
    pointer_size arrayIndex = static_cast<pointer_size>(index) >> SHIFT_BITS;
    pointer_size bitElement = static_cast<pointer_size>(index) & AND_BITS;
    return ((dataArray[arrayIndex] >> bitElement) & 1) != 0;
}

inline void DynamicBitset::ReallocateSize(uint32 _newElementCount)
{
    if (_newElementCount <= elementCount)
        return;

    pointer_size* newDataArray = new pointer_size[(_newElementCount >> SHIFT_BITS) + 1];
    Memset(newDataArray, 0, sizeof(pointer_size) * ((_newElementCount >> SHIFT_BITS) + 1));
    if (dataArray)
    {
        Memcpy(newDataArray, dataArray, sizeof(pointer_size) * ((_newElementCount >> SHIFT_BITS) + 1));
    }
    elementCount = _newElementCount;
    SafeDeleteArray(dataArray);
    dataArray = newDataArray;
}

inline pointer_size* DynamicBitset::GetData() const
{
    return dataArray;
}
};

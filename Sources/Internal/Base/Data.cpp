#include "Base/Data.h"

namespace DAVA
{
Data::Data(uint32 _size)
    : data(0)
    , size(_size)
{
    data = new uint8[size];
}

Data::Data(uint8* _data, uint32 _size)
    : data(_data)
    , size(_size)
{
}

Data::~Data()
{
    SafeDeleteArray(data);
}
};

#include "DataStorage/DataStorage.h"
#include "ICloudKeyValue.h"

namespace DAVA
{

#if defined(__DAVAENGINE_APPLE__) && !defined(__DAVAENGINE_STEAM__)

IDataStorage* DataStorage::Create()
{
    return new ICloudKeyValue();
}

#endif
}

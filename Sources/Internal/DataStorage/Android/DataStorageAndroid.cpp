#include "DataStorage/DataStorage.h"
#include "SharedPreferences.h"

namespace DAVA
{

#if defined(__DAVAENGINE_ANDROID__)

IDataStorage* DataStorage::Create()
{
    return new SharedPreferences();
}

#endif
}

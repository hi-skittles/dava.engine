#include "DataStorage/DataStorage.h"

#if defined(__DAVAENGINE_LINUX__) && !defined(__DAVAENGINE_STEAM__)

#include "DataStorage/Linux/DataStorageLinux.h"

namespace DAVA
{
IDataStorage* DataStorage::Create()
{
    return new DataStorageLinux();
}

} // namespace DAVA

#endif // defined(__DAVAENGINE_LINUX__) && !defined(__DAVAENGINE_STEAM__)

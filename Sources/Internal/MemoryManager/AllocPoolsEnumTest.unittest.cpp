#include "UnitTests/UnitTests.h"
#include <MemoryManager/AllocPools.h>

#include <Base/GlobalEnum.h>

DAVA_TESTCLASS (AllocPoolsEnumTest)
{
    DAVA_TEST (CheckStringValuesTest)
    {
        const EnumMap* enumMap = GlobalEnumMap<DAVA::ePredefAllocPools>::Instance();
        TEST_VERIFY(enumMap->GetCount() == DAVA::PREDEF_POOL_COUNT);
        for (DAVA::int32 i = 0; i < DAVA::PREDEF_POOL_COUNT; ++i)
        {
            TEST_VERIFY(enumMap->ToString(i) != nullptr);
        }
    }
};

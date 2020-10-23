#include "DAVAEngine.h"

#include "UnitTests/UnitTests.h"

#include <memory>

// disable for now
#if __cplusplus >= 201500L

using namespace DAVA;

auto f() -> int;

auto f() -> int
{
    return 42;
}

DAVA_TESTCLASS (Cpp14Test)
{
    DAVA_TEST (CompileTest)
    {
        TEST_VERIFY(f() == 42);
    }

    DAVA_TEST (ScopeExit)
    {
        int32 i = 0;
        {
            SCOPE_EXIT
            {
                ++i;
            };
            TEST_VERIFY(0 == i);
        }
        TEST_VERIFY(1 == i);
    }

    DAVA_TEST (MakeUnique)
    {
        std::unique_ptr<int> ptr = std::make_unique<int>();
        *ptr = 10;
        TEST_VERIFY(10 == *(ptr.get()));

        int* raw_ptr = ptr.release();
        TEST_VERIFY(10 == *raw_ptr);
        delete raw_ptr;
    }
}
;

#endif

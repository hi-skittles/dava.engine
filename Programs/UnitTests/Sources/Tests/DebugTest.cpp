#include "DAVAEngine.h"
#include "Debug/Backtrace.h"
#include "UnitTests/UnitTests.h"

DAVA_TESTCLASS (DebugTest)
{
    DAVA_TEST (StackTraceTest)
    {
        using namespace DAVA;
        using namespace Debug;
        // just check that we don't crash
        Vector<void*> stackTrace = GetBacktrace();
        TEST_VERIFY(stackTrace.empty() || !stackTrace.empty());
    }
};

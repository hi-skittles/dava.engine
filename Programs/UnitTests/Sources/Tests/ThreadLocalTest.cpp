#include "DAVAEngine.h"
#include "UnitTests/UnitTests.h"

#include "Concurrency/ThreadLocalPtr.h"

using namespace DAVA;

// Sample class to store in thread local storage
class TlsClass
{
public:
    TlsClass(int32 i, const String& s)
        : intValue(i)
        , strValue(s)
    {
    }

    int Int() const
    {
        return intValue;
    }
    const DAVA::String& String() const
    {
        return strValue;
    }

private:
    int32 intValue;
    DAVA::String strValue;
};

ThreadLocalPtr<int32> tlsInt;
ThreadLocalPtr<float> tlsFloat;
ThreadLocalPtr<int64> tlsInt64;
ThreadLocalPtr<TlsClass> tlsClass;

DAVA_TESTCLASS (ThreadLocalTest)
{
    DAVA_TEST (ThreadLocalTestFunc)
    {
        // Set thread local variables in main thread,
        tlsInt.Reset(new int(1));
        tlsFloat.Reset(new float(4.0f));
        tlsInt64.Reset(new int64(1010));
        tlsClass.Reset(new TlsClass(4, "Main thread"));

        // Run another thread which changes values of thread local variables
        Thread* thread = Thread::Create(MakeFunction(this, &ThreadLocalTest::ThreadFunc));
        thread->Start();
        thread->Join();
        SafeRelease(thread);

        // Make check that main thread's local variables didn't changed by another threads
        TEST_VERIFY(*tlsInt == 1);
        TEST_VERIFY(*tlsFloat == 4.0f);
        TEST_VERIFY(*tlsInt64 == 1010);

        TEST_VERIFY(tlsClass->Int() == 4);
        TEST_VERIFY(tlsClass->String() == "Main thread");

        // Delete memory occupied by pointer thread local variables
        tlsInt.Reset();
        tlsFloat.Reset();
        tlsInt64.Reset();
        tlsClass.Reset();
    }

    void ThreadFunc()
    {
        // Set thread local variables in another thread
        tlsInt.Reset(new int(333));
        tlsFloat.Reset(new float(10.0f));
        tlsInt64.Reset(new int64(100123));
        tlsClass.Reset(new TlsClass(1024, "This is a test"));

        // Delete memory occupied by pointer thread local variables
        tlsInt.Reset();
        tlsFloat.Reset();
        tlsInt64.Reset();
        tlsClass.Reset();
    }
}
;

#include "UnitTests/UnitTests.h"

#include "Debug/Private/RingArray.h"

DAVA_TESTCLASS (RingArrayTest)
{
    class RefCounter
    {
    public:
        RefCounter()
            : refCount(0)
        {
        }

        RefCounter(const RefCounter& other)
        {
            *this = other;
        }

        RefCounter& operator=(const RefCounter& other)
        {
            refCount = other.refCount + 1;
            return *this;
        }

        DAVA::uint32 refCount;
    };

    DAVA_TEST (NonTriviallyCopyableTypesTest)
    {
        using namespace DAVA;

        const size_t numberOfElements = 1000;

        RingArray<RefCounter> x(numberOfElements);
        RingArray<RefCounter> y(numberOfElements);

        const size_t numberOfIters = 1000;
        size_t refCount = 0;

        for (size_t i = 0; i < numberOfIters; ++i)
        {
            for (const RefCounter& rc : x)
            {
                TEST_VERIFY(rc.refCount == refCount);
            }

            y = x;
            ++refCount;

            for (const RefCounter& rc : y)
            {
                TEST_VERIFY(rc.refCount == refCount);
            }

            x = y;
            ++refCount;
        }
    }

    DAVA_TEST (CrashTest)
    {
        using namespace DAVA;

        const size_t numberOfElements = 100;

        RingArray<String> x(numberOfElements);
        RingArray<String> y(numberOfElements);

        const size_t numberOfIters = 500;

        for (size_t i = 0; i < numberOfIters; ++i)
        {
            for (String& s : y)
            {
                s = std::to_string(std::rand());
            }

            x = y;

            int64 rand = std::rand() % 1000;

            while (rand-- >= 0)
            {
                TEST_VERIFY(!x.next().empty()); // do some random checking, so compiler will not optimize `x` away
            }
        }
    }
};
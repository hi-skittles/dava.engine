#include "UnitTests/UnitTests.h"

#include "Logger/Logger.h"
#include "Concurrency/Thread.h"
#include "Concurrency/Atomic.h"

#include <algorithm>
#include <numeric>
#include <random>

using namespace DAVA;

DAVA_TESTCLASS (LoggerConcurrentTest)
{
    DAVA_TEST (ConcurrentLoggerTest)
    {
        const size_t threadsNumber = 10;
        Vector<Thread*> threads(threadsNumber);

        const Vector<String> strings = {
            "short test string",
            "medium test string ________________________________",
            "short tst str",
            "long test string _________________________________________________________________________________________________________________________________________________________________________________________________________________________________________________________________________________________________________",
            "medium test string _________________________________",
            "short str",
            "another test string",
            "another long test __________________________________________________________________________________________________________________________________________________________________________________________________________________________________________________________________________________________________________________________________________",
            "s",
            "",
            "abc"
        };

        const size_t itersNumber = 50;

        Atomic<size_t> threadsFinished{ 0 };

        for (auto& t : threads)
        {
            t = Thread::Create([&strings, &threadsFinished, itersNumber] {
                for (size_t i = 0; i < itersNumber; ++i)
                {
                    Vector<size_t> shuffled(strings.size());
                    std::iota(shuffled.begin(), shuffled.end(), 0); // shuffle indices instead of strings to reduce mem usage
                    std::shuffle(shuffled.begin(), shuffled.end(), std::default_random_engine());
                    for (auto index : shuffled)
                    {
                        Logger::Debug(strings[index].c_str());
                    }
                }
                ++threadsFinished;
            });

            t->Start();
        }

        for (auto& t : threads)
        {
            t->Join();
            SafeRelease(t);
        }

        TEST_VERIFY(threadsFinished == threadsNumber);
    }
};
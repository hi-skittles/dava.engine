#include "UnitTests/UnitTests.h"

#include "Base/BaseTypes.h"
#include "Base/FastName.h"
#include "Concurrency/Thread.h"
#include "Concurrency/SyncBarrier.h"

using namespace DAVA;

DAVA_TESTCLASS (FastNameTest)
{
    DAVA_TEST (ConstructorTest)
    {
        String s1("string_s1");
        const char* s2 = "string_s2";
        String s3(s2);

        FastName fn1(s1);
        FastName fn2(s2);
        FastName fn3(s3);

        TEST_VERIFY(strcmp(fn1.c_str(), s1.c_str()) == 0);
        TEST_VERIFY(strcmp(fn2.c_str(), s2) == 0);
        TEST_VERIFY(strcmp(fn3.c_str(), s2) == 0);
        TEST_VERIFY(strcmp(fn3.c_str(), s3.c_str()) == 0);

        FastName fn4(fn1);

        TEST_VERIFY(strcmp(fn4.c_str(), fn1.c_str()) == 0);
    }

    DAVA_TEST (IsValidAndEmptyMemFnTest)
    {
        FastName fn;

        TEST_VERIFY(!fn.IsValid());
        TEST_VERIFY(fn.empty());

        fn = FastName("test");

        TEST_VERIFY(fn.IsValid());
        TEST_VERIFY(!fn.empty());
    }

    DAVA_TEST (FindTest)
    {
        const size_t len = 42;

        String s1(len, 'a');

        size_t p1 = 0;
        size_t p2 = 0;
        size_t p3 = 0;

        size_t pp1 = 0;
        size_t pp2 = 0;
        size_t pp3 = 0;

        FastName fnToSearch(s1);

        String strToFind("a");
        FastName fnToFind(strToFind);

        for (size_t i = 0; i < len; ++i)
        {
            p1 = fnToSearch.find(strToFind, i);
            TEST_VERIFY(p1 == i);

            p2 = fnToSearch.find(strToFind.c_str(), i);
            TEST_VERIFY(p2 == i);

            p3 = fnToSearch.find(fnToFind, i);
            TEST_VERIFY(p3 == i);

            String tmpStrToFind(len - i, 'a');
            FastName tmpFnToFind(tmpStrToFind);

            pp1 = fnToSearch.find(tmpStrToFind, i);
            TEST_VERIFY(pp1 == i);

            pp2 = fnToSearch.find(tmpStrToFind.c_str(), i);
            TEST_VERIFY(pp2 == i);

            pp3 = fnToSearch.find(tmpFnToFind, i);
            TEST_VERIFY(pp3 == i);
        }

        FastName fn;

        size_t np = fn.find("nothing");
        TEST_VERIFY(np == String::npos);
    }

    DAVA_TEST (CmpTest)
    {
        String s1("test1");
        String s2("test2");

        FastName fn1(s1);
        FastName fn2(s2);

        TEST_VERIFY(fn1 != fn2);
        TEST_VERIFY(!(fn1 == fn2));

        const char* s = "test";

        fn1 = fn2;
        FastName fn3 = fn2;

        TEST_VERIFY(fn1 == fn2);
        TEST_VERIFY(!(fn1 != fn2));
        TEST_VERIFY(!(fn1 < fn2));

        TEST_VERIFY(fn1 == fn3);
        TEST_VERIFY(!(fn1 != fn3));
        TEST_VERIFY(!(fn1 < fn3));
    }

    DAVA_TEST (ConcurrentConstructorTest)
    {
        const size_t threadsNum = 24;
        const size_t testsNum = 768;

        Array<Thread*, threadsNum> threads;
        Vector<Array<FastName, threadsNum>> fns(testsNum);

        SyncBarrier barrier(threadsNum);

        for (size_t i = 0; i < threads.size(); ++i)
        {
            threads[i] = Thread::Create([i, &fns, &barrier]() {
                for (size_t j = 0; j < fns.size(); ++j)
                {
                    fns[j][i] = FastName(std::to_string(j));
                    barrier.Wait();
                }
            });
            threads[i]->Start();
        }

        for (auto& thread : threads)
        {
            thread->Join();
        }

        for (size_t i = 0; i < testsNum; ++i)
        {
            for (size_t j = 1; j < threadsNum; ++j)
            {
                TEST_VERIFY(fns[i][j - 1].c_str() == fns[i][j].c_str());
            }
            TEST_VERIFY(strcmp(fns[i].back().c_str(), std::to_string(i).c_str()) == 0);
        }
    }
};

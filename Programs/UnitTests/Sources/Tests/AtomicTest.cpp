#include "DAVAEngine.h"

#include "UnitTests/UnitTests.h"
#include "Concurrency/Thread.h"

#include <thread>

DAVA_TESTCLASS (AtomicTest)
{
    DAVA_TEST (OperationTest)
    {
        using namespace DAVA;

        const int theGreatestNumber = 42;
        const int theRandomNumber = 7;

        Atomic<char> atom_char;
        Atomic<short> atom_short;
        Atomic<int> atom_int;
        Atomic<int64> atom_int64;

        //ititial values
        TEST_VERIFY(atom_char == 0);
        TEST_VERIFY(atom_short == 0);
        TEST_VERIFY(atom_int == 0);
        TEST_VERIFY(atom_int64 == 0);

        //assigning
        atom_char = theGreatestNumber;
        atom_short = theGreatestNumber;
        atom_int = theGreatestNumber;
        atom_int64 = theGreatestNumber;

        TEST_VERIFY(atom_char == theGreatestNumber);
        TEST_VERIFY(atom_short == theGreatestNumber);
        TEST_VERIFY(atom_int == theGreatestNumber);
        TEST_VERIFY(atom_int64 == theGreatestNumber);

        //pre- & post- increment/decrement
        TEST_VERIFY(++atom_char == theGreatestNumber + 1);
        TEST_VERIFY(atom_char++ == theGreatestNumber + 1);
        TEST_VERIFY(atom_char == theGreatestNumber + 2);
        TEST_VERIFY(--atom_char == theGreatestNumber + 1);
        TEST_VERIFY(atom_char-- == theGreatestNumber + 1);
        TEST_VERIFY(atom_char == theGreatestNumber);

        TEST_VERIFY(++atom_short == theGreatestNumber + 1);
        TEST_VERIFY(atom_short++ == theGreatestNumber + 1);
        TEST_VERIFY(atom_short == theGreatestNumber + 2);
        TEST_VERIFY(--atom_short == theGreatestNumber + 1);
        TEST_VERIFY(atom_short-- == theGreatestNumber + 1);
        TEST_VERIFY(atom_short == theGreatestNumber);

        TEST_VERIFY(++atom_int == theGreatestNumber + 1);
        TEST_VERIFY(atom_int++ == theGreatestNumber + 1);
        TEST_VERIFY(atom_int == theGreatestNumber + 2);
        TEST_VERIFY(--atom_int == theGreatestNumber + 1);
        TEST_VERIFY(atom_int-- == theGreatestNumber + 1);
        TEST_VERIFY(atom_int == theGreatestNumber);

        TEST_VERIFY(++atom_int64 == theGreatestNumber + 1);
        TEST_VERIFY(atom_int64++ == theGreatestNumber + 1);
        TEST_VERIFY(atom_int64 == theGreatestNumber + 2);
        TEST_VERIFY(--atom_int64 == theGreatestNumber + 1);
        TEST_VERIFY(atom_int64-- == theGreatestNumber + 1);
        TEST_VERIFY(atom_int64 == theGreatestNumber);

        //Swap
        TEST_VERIFY(atom_char.Swap(0) == theGreatestNumber);
        TEST_VERIFY(atom_char != theGreatestNumber);
        TEST_VERIFY(atom_short.Swap(0) == theGreatestNumber);
        TEST_VERIFY(atom_short != theGreatestNumber);
        TEST_VERIFY(atom_int.Swap(0) == theGreatestNumber);
        TEST_VERIFY(atom_int != theGreatestNumber);
        TEST_VERIFY(atom_int64.Swap(0) == theGreatestNumber);
        TEST_VERIFY(atom_int64 != theGreatestNumber);

        //CAS
        atom_char = theRandomNumber;
        atom_short = theRandomNumber;
        atom_int = theRandomNumber;
        atom_int64 = theRandomNumber;

        TEST_VERIFY(atom_char.CompareAndSwap(1, 2) == false);
        TEST_VERIFY(atom_char.CompareAndSwap(theRandomNumber, theGreatestNumber));
        TEST_VERIFY(atom_char == theGreatestNumber);
        TEST_VERIFY(atom_short.CompareAndSwap(1, 2) == false);
        TEST_VERIFY(atom_short.CompareAndSwap(theRandomNumber, theGreatestNumber));
        TEST_VERIFY(atom_short == theGreatestNumber);
        TEST_VERIFY(atom_int.CompareAndSwap(1, 2) == false);
        TEST_VERIFY(atom_int.CompareAndSwap(theRandomNumber, theGreatestNumber));
        TEST_VERIFY(atom_int == theGreatestNumber);
        TEST_VERIFY(atom_int64.CompareAndSwap(1, 2) == false);
        TEST_VERIFY(atom_int64.CompareAndSwap(theRandomNumber, theGreatestNumber));
        TEST_VERIFY(atom_int64 == theGreatestNumber);
    }

    DAVA_TEST (MultiThreadedEnvironmentTest)
    {
        using namespace DAVA;

        const unsigned threadCount = std::thread::hardware_concurrency();
        const unsigned cycles = 10000;
        const unsigned targetNumber = cycles * threadCount;
        Atomic<long> resultNumber{ 0U };

        Vector<Thread*> threads(threadCount);
        for (auto& x : threads)
        {
            x = Thread::Create([&]
                               {
                                   for (size_t i = 0; i < cycles; ++i)
                                   {
                                       resultNumber++;
                                   }
                               });
            x->Start();
        }

        for (auto& x : threads)
        {
            if (x->IsJoinable())
            {
                x->Join();
            }
            SafeRelease(x);
        }

        TEST_VERIFY(resultNumber == targetNumber);
    }
};

#include "DAVAEngine.h"
#include "UnitTests/UnitTests.h"

using namespace DAVA;

class ObjectWithNDOverload
{
public:
    Vector3 position;
    Vector3 direction;
    Quaternion orientation;

    static void* operator new(size_t size);
    static void operator delete(void* pointer, size_t size);
    static FixedSizePoolAllocator pool;
};

FixedSizePoolAllocator ObjectWithNDOverload::pool(sizeof(ObjectWithNDOverload), 64);

void* ObjectWithNDOverload::operator new(size_t size)
{
    return pool.New();
}

void ObjectWithNDOverload::operator delete(void* pointer, size_t size)
{
    return pool.Delete(pointer);
}

class ObjectWithoutNDOverload
{
public:
    Vector3 position;
    Vector3 direction;
    Quaternion orientation;
};

DAVA_TESTCLASS (MemoryAllocatorsTest)
{
    DAVA_TEST (PoolAllocatorTest)
    {
        // 32 bytes block, 64 elements
        FixedSizePoolAllocator pool(32, 64);

        uint8* pointers[128];

        for (uint32 k = 0; k < 128; ++k)
        {
            pointers[k] = static_cast<uint8*>(pool.New());
            //Logger::Debug("ptr: %p", pointers[k]);
        }

        for (uint32 k = 1; k < 128; ++k)
        {
            if (k != 64)
                TEST_VERIFY(pointers[k] == pointers[k - 1] + 32);
            //        if (k != 64)
            //        if (pointers[k] != pointers[k - 1] + 32)
            //            Logger::Debug("Allocator error");
        }

        for (uint32 k = 0; k < 128; ++k)
        {
            pool.Delete(pointers[k]);
        }
    }

    DAVA_TEST (PoolAllocatorNewDeleteTest)
    {
        ObjectWithNDOverload* object1 = new ObjectWithNDOverload;
        SafeDelete(object1);

        ObjectWithNDOverload* object2 = new ObjectWithNDOverload;
        SafeDelete(object2);
    }
}
;

#ifndef __DAVAENGINE_ALLOCATOR_FACTORY_H__
#define __DAVAENGINE_ALLOCATOR_FACTORY_H__

#include "Base/BaseTypes.h"
#include "Base/Singleton.h"
#include "Base/FixedSizePoolAllocator.h"

#define IMPLEMENT_POOL_ALLOCATOR(TYPE, poolSize) \
	void* operator new(std::size_t size) \
	{ \
        DVASSERT(size == sizeof(TYPE)); /*probably you are allocating child class*/ \
		static FixedSizePoolAllocator* alloc = AllocatorFactory::Instance()->GetAllocator(typeid(TYPE).name(), sizeof(TYPE), poolSize); \
		return alloc->New(); \
	} \
	 \
	void operator delete(void* ptr) \
	{ \
		static FixedSizePoolAllocator* alloc = AllocatorFactory::Instance()->GetAllocator(typeid(TYPE).name(), sizeof(TYPE), poolSize); \
		alloc->Delete(ptr); \
	}

namespace DAVA
{
class AllocatorFactory : public Singleton<AllocatorFactory>
{
public:
    AllocatorFactory();
    virtual ~AllocatorFactory();

    FixedSizePoolAllocator* GetAllocator(const String& className, uint32 classSize, int32 poolLength);

    void Dump();

private:
    Map<String, FixedSizePoolAllocator*> allocators;
};
};

#endif //__DAVAENGINE_ALLOCATOR_FACTORY_H__
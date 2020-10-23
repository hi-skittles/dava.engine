#ifndef __DAVAENGINE_DYNAMIC_OBJECT_CACHE_H__
#define __DAVAENGINE_DYNAMIC_OBJECT_CACHE_H__

#include "Base/BaseTypes.h"
#include "Debug/DVAssert.h"

namespace DAVA
{
template <class T>
class DynamicObjectCacheData
{
public:
    DynamicObjectCacheData(int size);
    ~DynamicObjectCacheData();

    T* New();
    void Delete(T* _item);

    void AddToCache(int size);
    void Reset();

private:
    void CreateNewDataBlock();
    void DeallocateMemory();
    int32 size;
    List<uint8*> blocks;
    List<uint8*> cache;
};

template <class T>
class DynamicObjectCache
{
public:
    DynamicObjectCache(DynamicObjectCacheData<T>* data);
    ~DynamicObjectCache();

    T* New()
    {
        return data->New();
    }
    void Delete(T* _item)
    {
        return data->Delete(_item);
    }
    void Reset()
    {
        data->Reset();
    }

private:
    DynamicObjectCacheData<T>* data;
};

template <class T>
DynamicObjectCacheData<T>::DynamicObjectCacheData(int _size)
{
    size = _size;
    CreateNewDataBlock();
}

template <class T>
void DynamicObjectCacheData<T>::CreateNewDataBlock()
{
    uint8* block = new uint8[size * sizeof(T)];
    blocks.push_back(block);
    for (int k = 0; k < size; ++k)
    {
        cache.push_back(&block[k * sizeof(T)]);
    }
}

template <class T>
void DynamicObjectCacheData<T>::Reset()
{
    cache.clear();
    typename List<uint8*>::iterator end = blocks.end();
    for (typename List<uint8*>::iterator bit = blocks.begin(); bit != end; ++bit)
    {
        uint8* block = *bit;
        for (int k = 0; k < size; ++k)
        {
            cache.push_back(&block[k * sizeof(T)]);
        }
    }
}

template <class T>
void DynamicObjectCacheData<T>::DeallocateMemory()
{
    while (blocks.size() > 0)
    {
        uint8* object = blocks.front();
        blocks.pop_front();

        delete[] object;
    }
}

template <class T>
DynamicObjectCacheData<T>::~DynamicObjectCacheData()
{
    DeallocateMemory();
}

/*template <class T> 
void DynamicObjectCacheData<T>::AddToCache(int size)
{
	for (int k = 0; k < size; ++k)
		data.push_back(New());
}
 */

template <class T>
T* DynamicObjectCacheData<T>::New()
{
    T* object = 0;
    if (cache.empty())
        CreateNewDataBlock();

    DVASSERT(cache.size() > 0);

    void* ptr = cache.front();
    object = new (ptr) T(); //new (cache.front()) T;
    cache.pop_front();
    return object;
}

template <class T>
void DynamicObjectCacheData<T>::Delete(T* object)
{
    object->~T();
    cache.push_back(reinterpret_cast<uint8*>(object));
}
};

#endif // __DAVAENGINE_DYNAMIC_OBJECT_CACHE_H__
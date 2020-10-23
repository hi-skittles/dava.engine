#pragma once

#include "Base/BaseTypes.h"
#include "Debug/DVAssert.h"

namespace sl
{
class Allocator
{
public:
    template <typename T>
    T* New()
    {
        return reinterpret_cast<T*>(malloc(sizeof(T)));
    }
    template <typename T>
    T* New(size_t count)
    {
        return reinterpret_cast<T*>(malloc(sizeof(T) * count));
    }
    template <typename T>
    void Delete(T* ptr)
    {
        free(reinterpret_cast<void*>(ptr));
    }
    template <typename T>
    T* Realloc(T* ptr, size_t count)
    {
        return reinterpret_cast<T*>(realloc(ptr, sizeof(T) * count));
    }
};

DAVA::int32 String_PrintfArgList(char* buffer, int size, const char* format, va_list args);
DAVA::int32 String_FormatFloat(char* buffer, int size, float value);
bool String_Equal(const char* a, const char* b);
bool String_EqualNoCase(const char* a, const char* b);
double String_ToDouble(const char* str, char** end);
int String_ToInteger(const char* str, char** end);

template <typename T>
void ConstructRange(T* buffer, int new_size, int old_size)
{
    for (int i = old_size; i < new_size; i++)
    {
        new (buffer + i) T; // placement new
    }
}

template <typename T>
void ConstructRange(T* buffer, int new_size, int old_size, const T& val)
{
    for (int i = old_size; i < new_size; i++)
    {
        new (buffer + i) T(val); // placement new
    }
}

template <typename T>
void DestroyRange(T* buffer, int new_size, int old_size)
{
    for (int i = new_size; i < old_size; i++)
    {
        (buffer + i)->~T(); // Explicit call to the destructor
    }
}

template <typename T>
class Array
{
public:
    Array(Allocator* allocator)
        : allocator(allocator)
        , buffer(NULL)
        , m_size(0)
        , capacity(0)
    {
    }

    ~Array()
    {
        if (buffer)
        {
            allocator->Delete<T>(buffer);
            buffer = nullptr;
        }
    }

    void PushBack(const T& val)
    {
        DVASSERT(&val < buffer || &val >= buffer + m_size);

        int old_size = m_size;
        int new_size = m_size + 1;

        SetSize(new_size);

        ConstructRange(buffer, new_size, old_size, val);
    }
    T& PushBackNew()
    {
        int old_size = m_size;
        int new_size = m_size + 1;

        SetSize(new_size);

        ConstructRange(buffer, new_size, old_size);

        return buffer[old_size];
    }
    void Resize(int new_size)
    {
        int old_size = m_size;

        DestroyRange(buffer, new_size, old_size);

        SetSize(new_size);

        ConstructRange(buffer, new_size, old_size);
    }

    int GetSize() const
    {
        return m_size;
    }
    const T& operator[](int i) const
    {
        DVASSERT(i < m_size);
        return buffer[i];
    }
    T& operator[](int i)
    {
        DVASSERT(i < m_size);
        return buffer[i];
    }

private:
    // Change array size.
    void SetSize(int new_size)
    {
        m_size = new_size;

        if (new_size > capacity)
        {
            int new_buffer_size;
            if (capacity == 0)
            {
                // first allocation is exact
                new_buffer_size = new_size;
            }
            else
            {
                // following allocations grow array by 25%
                new_buffer_size = new_size + (new_size >> 2);
            }

            SetCapacity(new_buffer_size);
        }
    }

    // Change array capacity.
    void SetCapacity(int new_capacity)
    {
        DVASSERT(new_capacity >= m_size);

        if (new_capacity == 0)
        {
            // free the buffer.
            if (buffer != NULL)
            {
                allocator->Delete<T>(buffer);
                buffer = NULL;
            }
        }
        else
        {
            // realloc the buffer
            buffer = allocator->Realloc<T>(buffer, new_capacity);
        }

        capacity = new_capacity;
    }

private:
    Allocator* allocator; // @@ Do we really have to keep a pointer to this?
    T* buffer;
    int m_size;
    int capacity;
};

// Engine/StringPool.h

// @@ Implement this with a hash table!
struct StringPool
{
    StringPool(Allocator* allocator);
    ~StringPool();
    const char* AddString(const char* string);
    bool GetContainsString(const char* string) const;

    Array<char*> stringArray;
};

enum Target
{
    TARGET_VERTEX,
    TARGET_FRAGMENT
};



#if defined(__DAVAENGINE_POSIX__)

#define stricmp strcasecmp
#define strnicmp strncasecmp

#endif

#if defined(__DAVAENGINE_WINDOWS__)

    #define stricmp _strcmpi

#endif

} // namespace sl

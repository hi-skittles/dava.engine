#ifndef __DAVAENGINE_FAST_NAME__
#define __DAVAENGINE_FAST_NAME__

#include "Base/Hash.h"
#include "Base/Any.h"
#include "Concurrency/Spinlock.h"

namespace DAVA
{
class FastNameDB final
{
    friend class FastName;

public:
    using MutexT = Spinlock;
    using CharT = char;

    static FastNameDB* GetLocalDB();
    void SetMasterDB(FastNameDB* masterDB);

private:
    FastNameDB()
        : nameToIndexMap(8192 * 2)
    {
        namesTable.reserve(512);
    }

    ~FastNameDB()
    {
        const size_t count = namesTable.size();
        for (size_t i = 0; i < count; ++i)
        {
            SafeDeleteArray(namesTable[i]);
        }
    }

    static FastNameDB** GetLocalDBPtr();

    struct FastNameDBHash
    {
        size_t operator()(const char* str) const
        {
            return DavaHashString(str);
        }
    };

    struct FastNameDBEqualTo
    {
        bool operator()(const char* left, const char* right) const
        {
            return (0 == strcmp(left, right));
        }
    };

    Vector<const CharT*> namesTable;
    UnorderedMap<const CharT*, size_t, FastNameDBHash, FastNameDBEqualTo> nameToIndexMap;

    MutexT mutex;
    size_t sizeOfNames = 0;
};

class FastName
{
public:
    FastName();
    explicit FastName(const char* name);
    explicit FastName(const String& name);

    bool operator<(const FastName& _name) const;
    bool operator==(const FastName& _name) const;
    bool operator!=(const FastName& _name) const;

    bool empty() const;
    const char* c_str() const;
    size_t find(const char* s, size_t pos = 0) const;
    size_t find(const String& str, size_t pos = 0) const;
    size_t find(const FastName& fn, size_t pos = 0) const;

    bool IsValid() const;

private:
    void Init(const char* name);
    const char* str = nullptr;
};

inline FastName::FastName() = default;

inline FastName::FastName(const String& name)
{
    Init(name.c_str());
}

inline FastName::FastName(const char* name)
{
    Init(name);
}

inline bool FastName::operator==(const FastName& _name) const
{
    return str == _name.str;
}

inline bool FastName::operator!=(const FastName& _name) const
{
    return str != _name.str;
}

inline bool FastName::operator<(const FastName& _name) const
{
    return str < _name.str;
}

inline bool FastName::empty() const
{
    return str == nullptr;
}

inline size_t FastName::find(const char* s, size_t pos) const
{
    if (nullptr != str && nullptr != s)
    {
        DVASSERT(pos <= strlen(str));

        const char* q = strstr(str + pos, s);
        return q ? (q - str) : String::npos;
    }
    return String::npos;
}

inline size_t FastName::find(const String& str, size_t pos) const
{
    return find(str.c_str(), pos);
}

inline size_t FastName::find(const FastName& fn, size_t pos) const
{
    return find(fn.c_str(), pos);
}

inline bool FastName::IsValid() const
{
    return !empty();
}

inline const char* FastName::c_str() const
{
    return str;
}

template <>
bool AnyCompare<FastName>::IsEqual(const Any& v1, const Any& v2);
extern template struct AnyCompare<FastName>;
};

namespace std
{
template <>
struct hash<DAVA::FastName>
{
    size_t operator()(const DAVA::FastName& k) const
    {
        return reinterpret_cast<size_t>(k.c_str());
    }
};
}

#endif // __DAVAENGINE_FAST_NAME__

#include "FastName.h"
#include "Debug/DVAssert.h"

#include "Concurrency/LockGuard.h"

namespace DAVA
{
FastNameDB* FastNameDB::GetLocalDB()
{
    return *GetLocalDBPtr();
}

FastNameDB** FastNameDB::GetLocalDBPtr()
{
    static FastNameDB db;
    static FastNameDB* dbPtr = &db;
    return &dbPtr;
}

void FastNameDB::SetMasterDB(FastNameDB* db)
{
    static bool hasMaster = false;

    DVASSERT(!hasMaster);
    hasMaster = true;

    // Override db from local to master
    FastNameDB** localDBPtr = GetLocalDBPtr();
    *localDBPtr = db;
}

void FastName::Init(const char* name)
{
    DVASSERT(nullptr != name);

    FastNameDB* db = FastNameDB::GetLocalDB();
    LockGuard<FastNameDB::MutexT> guard(db->mutex);

    // search if that name is already in hash
    auto it = db->nameToIndexMap.find(name);
    if (it != db->nameToIndexMap.end())
    {
        // already exist, so we just need to set the same index to this object
        str = db->namesTable[it->second];
    }
    else
    {
        // string isn't in hash and it isn't in names table, so we need to copy it
        // and find place for copied string in names table and set it
        size_t nameLen = strlen(name);
        FastNameDB::CharT* nameCopy = new FastNameDB::CharT[nameLen + 1];
        memcpy(nameCopy, name, nameLen + 1);

        db->sizeOfNames += (nameLen * sizeof(FastNameDB::CharT));

        // index will be a new row in names table
        size_t index = db->namesTable.size();

        db->namesTable.push_back(nameCopy);
        db->nameToIndexMap.emplace(nameCopy, index);

        str = nameCopy;
    }
}

template <>
bool AnyCompare<FastName>::IsEqual(const Any& v1, const Any& v2)
{
    return v1.Get<FastName>() == v2.Get<FastName>();
}

} // namespace DAVA

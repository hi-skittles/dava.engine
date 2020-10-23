#include "Reflection/ReflectedTypeDB.h"
#include "Reflection/Private/Wrappers/StructureWrapperClass.h"

namespace DAVA
{
ReflectedTypeDB* ReflectedTypeDB::GetLocalDB()
{
    return *GetLocalDBPtr();
}

ReflectedTypeDB** ReflectedTypeDB::GetLocalDBPtr()
{
    static ReflectedTypeDB db;
    static ReflectedTypeDB* dbPtr = &db;
    return &dbPtr;
}

void ReflectedTypeDB::SetMasterDB(ReflectedTypeDB* db)
{
    static bool hasMaster = false;

    DVASSERT(!hasMaster);
    hasMaster = true;

    // Override db from local to master
    ReflectedTypeDB** localDBPtr = GetLocalDBPtr();
    *localDBPtr = db;
}

void ReflectedTypeDB::RegisterDBType(ReflectedType* r)
{
    ReflectedTypeDB* db = GetLocalDB();
    if (db->typeToReflectedTypeMap.find(r->type) == db->typeToReflectedTypeMap.end())
    {
        db->typeToReflectedTypeMap[r->type] = r;
        db->typeNameToReflectedTypeMap[String(r->type->GetName())] = r;
    }
    else
    {
        //TODO: check incoming types for dll's
    }
}

const ReflectedType* ReflectedTypeDB::GetByPointer(const void* ptr, const Type* derefType)
{
    DVASSERT(nullptr != ptr);
    DVASSERT(nullptr != derefType);
    DVASSERT(!derefType->IsPointer());

    Type::SeedCastOP seedOP = derefType->GetSeedCastOP();

    if (nullptr != seedOP)
    {
        const ReflectionBase* rb = static_cast<const ReflectionBase*>((*seedOP)(ptr));
        return ReflectedTypeDBDetail::GetVirtualReflectedType(rb);
    }

    return GetByType(derefType);
}

const ReflectedType* ReflectedTypeDB::GetByType(const Type* type)
{
    const ReflectedType* ret = nullptr;

    auto it = GetLocalDB()->typeToReflectedTypeMap.find(type);
    if (it != GetLocalDB()->typeToReflectedTypeMap.end())
    {
        ret = it->second;
    }

    return ret;
}

const ReflectedType* ReflectedTypeDB::GetByTypeName(const String& rttiName)
{
    const ReflectedType* ret = nullptr;

    auto it = GetLocalDB()->typeNameToReflectedTypeMap.find(rttiName);
    if (it != GetLocalDB()->typeNameToReflectedTypeMap.end())
    {
        ret = it->second;
    }

    return ret;
}

const ReflectedType* ReflectedTypeDB::GetByPermanentName(const String& permanentName)
{
    const ReflectedType* ret = nullptr;

    auto it = GetLocalDB()->permanentNameToReflectedTypeMap.find(permanentName);
    if (it != GetLocalDB()->permanentNameToReflectedTypeMap.end())
    {
        ret = it->second;
    }

    return ret;
}

ReflectedType* ReflectedTypeDB::CreateCustomType(const Type* type, const String& permanentName)
{
    GetLocalDB()->customReflectedTypes.emplace_back(new ReflectedType(type));
    ReflectedType* ret = GetLocalDB()->customReflectedTypes.back().get();

    String rttiName(type->GetName());

    DVASSERT(GetLocalDB()->typeToReflectedTypeMap.count(type) == 0 && "ReflectedType with specified RttiType already exists");
    DVASSERT(GetLocalDB()->typeNameToReflectedTypeMap.count(rttiName) == 0 && "ReflectedType with specified RttiType::name already exists");

    GetLocalDB()->typeToReflectedTypeMap[type] = ret;
    GetLocalDB()->typeNameToReflectedTypeMap[rttiName] = ret;

    RegisterPermanentName(ret, permanentName);

    return ret;
}

void ReflectedTypeDB::RegisterPermanentName(const ReflectedType* reflectedType, const String& permanentName)
{
    ReflectedType* rt = const_cast<ReflectedType*>(reflectedType);

    DVASSERT(rt != nullptr);
    if (rt->permanentName == permanentName)
    {
        return;
    }

    DVASSERT(rt->permanentName.empty() && "Name is already set");
    DVASSERT(GetLocalDB()->permanentNameToReflectedTypeMap.count(permanentName) == 0 && "Permanent name alredy in use");

    rt->permanentName = permanentName;
    GetLocalDB()->permanentNameToReflectedTypeMap[permanentName] = rt;
}

ReflectedTypeDB::Stats ReflectedTypeDB::GetLocalDBStats()
{
    ReflectedTypeDB::Stats stats;

    stats.reflectedTypeCount = GetLocalDB()->typeToReflectedTypeMap.size();
    stats.reflectedTypeMemory = stats.reflectedTypeCount * sizeof(ReflectedType);

    for (auto& p : GetLocalDB()->typeToReflectedTypeMap)
    {
        const ReflectedType* tr = p.second;
        if (tr->structure != nullptr)
        {
            stats.reflectedStructCount++;
            stats.reflectedStructFieldsCount += tr->structure->fields.size();
            stats.reflectedStructMethodsCount += tr->structure->methods.size();
            stats.reflectedStructEnumsCount += tr->structure->enums.size();
            stats.reflectedStructCtorsCount += tr->structure->ctors.size();

            if (tr->structure->dtor != nullptr)
                stats.reflectedStructDtorsCount++;

            if (tr->structure->meta != nullptr)
            {
                stats.reflectedStructMetasCount++;
                stats.reflectedStructMetaMCount += tr->structure->meta->metas.size();
            }
        }

        if (tr->structureWrapper != nullptr)
        {
            stats.reflectedStructWrapperCount++;

            StructureWrapperClass* swc = dynamic_cast<StructureWrapperClass*>(tr->structureWrapper.get());
            if (nullptr != swc)
            {
                stats.reflectedStructWrapperClassCount++;
                stats.reflectedStructWrapperClassMemory +=
                sizeof(StructureWrapperClass) +
                swc->fieldsCache.size() * sizeof(StructureWrapperClass::CachedFieldEntry) +
                swc->methodsCache.size() * sizeof(StructureWrapperClass::CachedMethodEntry) +
                swc->fieldsNameIndexes.size() * sizeof(decltype(swc->fieldsNameIndexes)::key_type) +
                swc->fieldsNameIndexes.size() * sizeof(decltype(swc->fieldsNameIndexes)::mapped_type) +
                swc->methodsNameIndexes.size() * sizeof(decltype(swc->methodsNameIndexes)::key_type) +
                swc->methodsNameIndexes.size() * sizeof(decltype(swc->methodsNameIndexes)::mapped_type);
            }
        }
    }

    stats.reflectedTypeDBMemory =
    sizeof(ReflectedTypeDB) +
    sizeof(decltype(typeToReflectedTypeMap)) +
    sizeof(decltype(typeNameToReflectedTypeMap)) +
    sizeof(decltype(permanentNameToReflectedTypeMap)) +
    GetLocalDB()->typeToReflectedTypeMap.size() * sizeof(decltype(ReflectedTypeDB::typeToReflectedTypeMap)::key_type) +
    GetLocalDB()->typeToReflectedTypeMap.size() * sizeof(decltype(ReflectedTypeDB::typeToReflectedTypeMap)::mapped_type) +
    GetLocalDB()->typeNameToReflectedTypeMap.size() * sizeof(decltype(ReflectedTypeDB::typeNameToReflectedTypeMap)::key_type) +
    GetLocalDB()->typeNameToReflectedTypeMap.size() * sizeof(decltype(ReflectedTypeDB::typeNameToReflectedTypeMap)::mapped_type) +
    GetLocalDB()->permanentNameToReflectedTypeMap.size() * sizeof(decltype(ReflectedTypeDB::permanentNameToReflectedTypeMap)::key_type) +
    GetLocalDB()->permanentNameToReflectedTypeMap.size() * sizeof(decltype(ReflectedTypeDB::permanentNameToReflectedTypeMap)::mapped_type);

    stats.reflectedStructMemory =
    stats.reflectedStructCount * sizeof(ReflectedStructure) +
    stats.reflectedStructCtorsCount * sizeof(AnyFn) +
    stats.reflectedStructDtorsCount * sizeof(AnyFn) +
    stats.reflectedStructEnumsCount * sizeof(ReflectedStructure::Enum) +
    stats.reflectedStructFieldsCount * sizeof(ReflectedStructure::Field) +
    stats.reflectedStructMethodsCount * sizeof(ReflectedStructure::Method) +
    stats.reflectedStructMetasCount * sizeof(ReflectedMeta) +
    stats.reflectedStructMetaMCount * sizeof(Any);

    stats.totalMemory =
    stats.reflectedTypeMemory +
    stats.reflectedTypeDBMemory +
    stats.reflectedStructMemory +
    stats.reflectedStructWrapperClassMemory;

    return stats;
}

} // namespace DAVA

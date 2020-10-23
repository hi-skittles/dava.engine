#pragma once

#include "Base/List.h"
#include "Base/UnordererMap.h"

#include "Reflection/Reflection.h"
#include "Reflection/ReflectedType.h"
#include "Reflection/ReflectedMeta.h"
#include "Reflection/ReflectedStructure.h"

namespace DAVA
{
class Type;
class ReflectedType;

/**
    \ingroup reflection
    ReflectedType database.

    Holds info about type static structure `ReflectedStructure` and runtime structure wrapper `StructureWrapper`.
*/
class ReflectedTypeDB
{
    template <typename C>
    friend class ReflectionRegistrator;

public:
    struct Stats
    {
        size_t reflectedTypeCount = 0;
        size_t reflectedTypeMemory = 0;
        size_t reflectedStructCount = 0;
        size_t reflectedStructWrapperCount = 0;
        size_t reflectedStructWrapperClassCount = 0;
        size_t reflectedStructWrapperClassMemory = 0;
        size_t reflectedStructFieldsCount = 0;
        size_t reflectedStructMethodsCount = 0;
        size_t reflectedStructEnumsCount = 0;
        size_t reflectedStructCtorsCount = 0;
        size_t reflectedStructDtorsCount = 0;
        size_t reflectedStructMetasCount = 0;
        size_t reflectedStructMetaMCount = 0;
        size_t reflectedStructMemory = 0;
        size_t reflectedTypeDBMemory = 0;
        size_t totalMemory = 0;
    };

    template <typename T>
    static const ReflectedType* Get();

    template <typename T>
    static const ReflectedType* GetByPointer(const T* ptr);
    static const ReflectedType* GetByPointer(const void* ptr, const Type* derefType);

    static const ReflectedType* GetByType(const Type* type);
    static const ReflectedType* GetByTypeName(const String& rttiName);
    static const ReflectedType* GetByPermanentName(const String& permanentName);

    static ReflectedType* CreateCustomType(const Type* type, const String& permanentName);

    template <typename T, typename... Bases>
    static void RegisterBases();
    static void RegisterPermanentName(const ReflectedType* reflectedType, const String& permanentName);

    static ReflectedTypeDB* GetLocalDB();
    static Stats GetLocalDBStats();

    void SetMasterDB(ReflectedTypeDB* masterDb);

protected:
    template <typename T>
    static ReflectedType* CreateStatic();

    template <typename T>
    static ReflectedType* Edit();

    static ReflectedTypeDB** GetLocalDBPtr();

    static void RegisterDBType(ReflectedType* reflectedType);

    List<std::unique_ptr<ReflectedType>> customReflectedTypes;
    UnorderedMap<const Type*, ReflectedType*> typeToReflectedTypeMap;
    UnorderedMap<String, ReflectedType*> typeNameToReflectedTypeMap;
    UnorderedMap<String, ReflectedType*> permanentNameToReflectedTypeMap;
};
} // namespace DAVA

#ifndef __DAVA_Reflection__
#define __DAVA_Reflection__
#endif
#include "Reflection/Private/Reflection_impl.h"
#include "Reflection/Private/ReflectedObject_impl.h"
#include "Reflection/Private/ReflectedMeta_impl.h"
#include "Reflection/Private/ReflectedType_impl.h"
#include "Reflection/Private/ReflectedTypeDB_impl.h"

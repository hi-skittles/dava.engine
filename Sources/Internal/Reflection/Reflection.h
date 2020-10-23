#pragma once

#include <tuple>
#include <ostream>

#include "Base/Any.h"
#include "Base/AnyFn.h"
#include "Base/Type.h"
#include "Base/FastName.h"

#include "Debug/DVAssert.h"
#include "Reflection/ReflectedObject.h"

// #include "Reflection/ReflectedMeta.h"
// #include "Reflection/ReflectedType.h"
// #include "Reflection/ReflectedTypeDB.h"
// #include "Reflection/ReflectedStructure.h"

/** \defgroup reflection Reflection
    TODO: detailed description 
*/

/**
    \ingroup reflection
    TODO: usage comments
*/
#define DAVA_REFLECTION(Cls) IMPL__DAVA_REFLECTION(Cls)

/**
    \ingroup reflection
    TODO: usage comments
*/
#define DAVA_VIRTUAL_REFLECTION(Cls, ...) IMPL__DAVA_VIRTUAL_REFLECTION(Cls, ##__VA_ARGS__)

/**
\ingroup reflection
TODO: usage comments
*/
#define DAVA_VIRTUAL_REFLECTION_IN_PLACE(Cls, ...) IMPL__DAVA_VIRTUAL_REFLECTION_IN_PLACE(Cls, ##__VA_ARGS__)

/**
\ingroup reflection
TODO: usage comments
*/
#define DAVA_REFLECTION_IMPL(Cls) IMPL__DAVA_REFLECTION_IMPL(Cls)

/**
\ingroup reflection
TODO: usage comments
*/
#define DAVA_VIRTUAL_REFLECTION_IMPL(Cls) IMPL__DAVA_VIRTUAL_REFLECTION_IMPL(Cls)

/**
\ingroup reflection
TODO: usage comments
*/
#define DAVA_VIRTUAL_TEMPLATE_REFLECTION_IMPL(Cls) IMPL__DAVA_VIRTUAL_TEMPLATE_REFLECTION_IMPL(Cls)

/**
 \ingroup reflection
 TODO: usage comments
 */
#define DAVA_VIRTUAL_TEMPLATE_SPECIALIZATION_REFLECTION_IMPL(Cls) IMPL__DAVA_VIRTUAL_TEMPLATE_SPECIALIZATION_REFLECTION_IMPL(Cls)

/**
    \ingroup reflection
    TODO: usage comments
*/
#define DAVA_REFLECTION_REGISTER_PERMANENT_NAME(Cls) IMPL__DAVA_REFLECTION_REGISTER_PERMANENT_NAME(Cls)

/**
    \ingroup reflection
    TODO: usage comments
*/
#define DAVA_REFLECTION_REGISTER_CUSTOM_PERMANENT_NAME(Cls, Name) IMPL__DAVA_REFLECTION_REGISTER_CUSTOM_PERMANENT_NAME(Cls, Name)

namespace DAVA
{
class ReflectedMeta;
class ValueWrapper;
class StructureWrapper;

/**
    \ingroup reflection
    Must be used as base class for any user class that is going to have virtual reflection.

    \code
    class Foo : public DAVA::Reflection
    {
        DAVA_VIRTUAL_REFLECTION(Foo);
        // ...
    };
    \endcode
*/
struct ReflectionBase : Type::Seed
{
    virtual ~ReflectionBase() = default;
    virtual const ReflectedType* Dava__GetReflectedType() const = 0;
};

/** 
    \ingroup reflection
    Holds reflected type information linked to appropriate runtime object.

    Reflection is created by linking any class or primitive data with its unique reflected type.
    Obtained Reflection allows to perform a number of operation over linked object:
    - get or set value from/to the object at runtime
    - introspect the object - its fields, methods, enumerations at runtime.
 
        +---------------+
        | ReflectedType |
        +---------------+

    It is also possible to create a new object or destroy existing objects from/with known reflected type.
*/
class Reflection final
{
public:
    struct Field;
    struct FieldCaps;
    struct Method;

    enum class CtorPolicy;

    Reflection() = default;
    Reflection(const Reflection&) = default;
    Reflection(const ReflectedObject& object, const ValueWrapper* vw, const StructureWrapper* sw, const ReflectedMeta* meta);

    bool IsValid() const;
    bool IsReadonly() const;

    const Type* GetValueType() const;
    ReflectedObject GetValueObject() const;
    ReflectedObject GetDirectObject() const;

    Any GetValue() const;
    bool SetValue(const Any& value) const;
    bool SetValueWithCast(const Any& value) const;

    bool HasFields() const;
    Reflection GetField(const Any& name) const;
    Vector<Field> GetFields() const;

    bool HasMethods() const;
    AnyFn GetMethod(const String& key) const;
    Vector<Method> GetMethods() const;

    void Dump(std::ostream& out, size_t deep = 0) const;

    template <typename T>
    const T* GetMeta() const;

    const void* GetMeta(const Type* metaType) const;

    template <typename T>
    static Reflection Create(T* objectPtr, const ReflectedMeta* objectMeta = nullptr);

    static Reflection Create(const ReflectedObject& object, const ReflectedMeta* objectMeta = nullptr);

    static Reflection Create(const Any& any, const ReflectedMeta* objectMeta = nullptr);

    DAVA_DEPRECATED(static Reflection Create(const Reflection& etalon, const Reflection& metaProvider));
    DAVA_DEPRECATED(static Reflection Create(const Reflection& etalon, const ReflectedMeta* objectMeta));

    //
    // Experimental API for fields add/remove/insert create.
    // Is subject of change!
    //
    // -->
    //
    const FieldCaps& GetFieldsCaps() const;
    bool AddField(const Any& key, const Any& value) const;
    bool InsertField(const Any& beforeKey, const Any& key, const Any& value) const;
    bool RemoveField(const Any& key) const;
    AnyFn GetFieldCreator() const;
    //
    // <--
    //

private:
    ReflectedObject object;
    const ValueWrapper* valueWrapper = nullptr;
    const StructureWrapper* structureWrapper = nullptr;
    const ReflectedMeta* meta = nullptr;
};

struct Reflection::Field
{
    Field() = default;
    Field(Any&&, Reflection&&, const ReflectedType*);

    Any key;
    Reflection ref;
    const ReflectedType* inheritFrom = nullptr;
};

struct Reflection::Method
{
    Method() = default;
    Method(Any key, AnyFn&&);

    Any key;
    AnyFn fn;
};

//
// Experimental API for fields add/remove/insert create.
// Is subject of change!
//
// -->
//
struct Reflection::FieldCaps
{
    bool canAddField = false;
    bool canInsertField = false;
    bool canRemoveField = false;
    bool canCreateFieldValue = false;
    bool hasFlatStruct = false; //< structure is flat - all keys have the same type, all values have the same type
    bool hasDynamicStruct = false; //< add/removing/inserting one value into that structure can cause changes to existing values
    bool hasRangeAccess = false; //< structure has deterministic size and its size can be retrieved, its fields can be accessed by range
    const Type* flatKeysType = nullptr; //< when structure if flat this will hold the key type, or `nullptr` if not
    const Type* flatValuesType = nullptr; //< when structure if flat this will hold the value type, or `nullptr` if not
};
//
// <--
//

class ValueWrapper
{
public:
    ValueWrapper() = default;
    ValueWrapper(const ValueWrapper&) = delete;
    virtual ~ValueWrapper() = default;

    virtual const Type* GetType(const ReflectedObject& object) const = 0;

    virtual bool IsReadonly(const ReflectedObject& object) const = 0;
    virtual Any GetValue(const ReflectedObject& object) const = 0;
    virtual bool SetValue(const ReflectedObject& object, const Any& value) const = 0;
    virtual bool SetValueWithCast(const ReflectedObject& object, const Any& value) const = 0;

    virtual ReflectedObject GetValueObject(const ReflectedObject& object) const = 0;
};

class EnumWrapper
{
    // TODO: implement
};

class StructureWrapper
{
public:
    StructureWrapper() = default;
    StructureWrapper(const StructureWrapper&) = delete;
    virtual ~StructureWrapper() = default;

    virtual void Update() = 0;

    virtual bool HasFields(const ReflectedObject& object, const ValueWrapper* vw) const = 0;
    virtual size_t GetFieldsCount(const ReflectedObject& object, const ValueWrapper* vw) const = 0;
    virtual Reflection GetField(const ReflectedObject& object, const ValueWrapper* vw, const Any& key) const = 0;
    virtual Vector<Reflection::Field> GetFields(const ReflectedObject& object, const ValueWrapper* vw) const = 0;
    virtual Vector<Reflection::Field> GetFields(const ReflectedObject& object, const ValueWrapper* vw, size_t first, size_t count) const = 0;

    virtual const Reflection::FieldCaps& GetFieldsCaps(const ReflectedObject& object, const ValueWrapper* vw) const = 0;

    virtual bool HasMethods(const ReflectedObject& object, const ValueWrapper* vw) const = 0;
    virtual AnyFn GetMethod(const ReflectedObject& object, const ValueWrapper* vw, const Any& key) const = 0;
    virtual Vector<Reflection::Method> GetMethods(const ReflectedObject& object, const ValueWrapper* vw) const = 0;

    virtual AnyFn GetFieldCreator(const ReflectedObject& object, const ValueWrapper* vw) const = 0;
    virtual bool AddField(const ReflectedObject& object, const ValueWrapper* vw, const Any& key, const Any& value) const = 0;
    virtual bool InsertField(const ReflectedObject& object, const ValueWrapper* vw, const Any& beforeKey, const Any& key, const Any& value) const = 0;
    virtual bool RemoveField(const ReflectedObject& object, const ValueWrapper* vw, const Any& key) const = 0;
};

template <typename T>
struct StructureWrapperCreator;

} // namespace DAVA

#ifndef __DAVA_Reflection__
#define __DAVA_Reflection__
#endif
#include "Reflection/Private/Reflection_pre_impl.h"

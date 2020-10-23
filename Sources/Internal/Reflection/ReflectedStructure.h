#pragma once

#include "Base/String.h"
#include "Base/Vector.h"
#include "Base/FastName.h"
#include "Base/AnyFn.h"
#include <memory>

namespace DAVA
{
class Type;
class ReflectedMeta;
class ReflectedType;
class ValueWrapper;
class MethodWrapper;
class EnumWrapper;
class CtorWrapper;
class DtorWrapper;

class ReflectedStructure final
{
public:
    using Key = FastName;

    struct Field
    {
        Key name;
        std::unique_ptr<ValueWrapper> valueWrapper;
        std::unique_ptr<ReflectedMeta> meta;
    };

    struct Method
    {
        Key name;
        AnyFn fn;
        std::unique_ptr<ReflectedMeta> meta;
    };

    struct Enum
    {
        Key name;
        std::unique_ptr<EnumWrapper> enumWrapper;
    };

    std::unique_ptr<ReflectedMeta> meta;

    Vector<std::unique_ptr<Field>> fields;
    Vector<std::unique_ptr<Method>> methods;
    Vector<std::unique_ptr<Enum>> enums;

    Vector<std::unique_ptr<AnyFn>> ctors;
    std::unique_ptr<AnyFn> dtor;
};
} // namespace DAVA

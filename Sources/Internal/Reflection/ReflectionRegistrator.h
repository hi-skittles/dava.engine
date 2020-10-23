#pragma once

#include "Reflection/Reflection.h"
#include "Reflection/ReflectedTypeDB.h"
#include "Reflection/Private/Wrappers/ValueWrapperDefault.h"
#include "Reflection/Private/Wrappers/ValueWrapperDirect.h"
#include "Reflection/Private/Wrappers/ValueWrapperClass.h"
#include "Reflection/Private/Wrappers/ValueWrapperClassFn.h"
#include "Reflection/Private/Wrappers/ValueWrapperClassFnPtr.h"
#include "Reflection/Private/Wrappers/ValueWrapperStatic.h"
#include "Reflection/Private/Wrappers/ValueWrapperStaticFn.h"
#include "Reflection/Private/Wrappers/ValueWrapperStaticFnPtr.h"
#include "Reflection/Private/Wrappers/ValueWrapperStaticFnPtrC.h"
#include "Reflection/Private/Wrappers/StructureWrapperClass.h"
#include "Reflection/Private/Wrappers/StructureWrapperPtr.h"
#include "Reflection/Private/Wrappers/StructureWrapperStdIdx.h"
#include "Reflection/Private/Wrappers/StructureWrapperStdSet.h"
#include "Reflection/Private/Wrappers/StructureWrapperStdMap.h"

namespace DAVA
{
/// \brief A reflection registration, that is used to register complex types structure.
template <typename C>
class ReflectionRegistrator final
{
public:
    ~ReflectionRegistrator();

    static ReflectionRegistrator Begin(std::unique_ptr<StructureWrapper>&& customStructureWrapper = std::unique_ptr<StructureWrapper>());

    template <typename... Args>
    ReflectionRegistrator& ConstructorByValue();

    template <typename... Args>
    ReflectionRegistrator& ConstructorByPointer();

    template <typename... Args>
    ReflectionRegistrator& ConstructorByPointer(C* (*fn)(Args...));

    ReflectionRegistrator& DestructorByPointer();

    ReflectionRegistrator& DestructorByPointer(void (*fn)(C*));

    template <typename T>
    ReflectionRegistrator& Field(const char* name, T* field);

    template <typename T>
    ReflectionRegistrator& Field(const char* name, T C::*field);

    template <typename GetF, typename SetF>
    ReflectionRegistrator& Field(const char* name, GetF getter, SetF setter = nullptr);

    template <typename Mt>
    ReflectionRegistrator& Method(const char* name, const Mt& method);

    ReflectionRegistrator& BindMeta(ReflectedMeta&& meta);

    ReflectionRegistrator& operator[](ReflectedMeta&& meta);

    void End();

private:
    ReflectionRegistrator(std::unique_ptr<StructureWrapper>&& customStructureWrapper);

    ReflectedStructure* structure = nullptr;
    std::unique_ptr<ReflectedMeta>* lastMeta;

    ReflectionRegistrator& AddField(const char* name, ReflectedStructure::Field* f);
};

} // namespace DAVA

#define __DAVA_Reflection_Registrator__
#include "Reflection/Private/ReflectionRegistrator_impl.h"

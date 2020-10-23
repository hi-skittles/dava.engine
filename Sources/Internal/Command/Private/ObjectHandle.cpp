#include "Command/ObjectHandle.h"

#include "Base/Meta.h"
#include "Base/IntrospectionBase.h"
#include "Base/Introspection.h"

#include "Debug/DVAssert.h"

namespace DAVA
{
ObjectHandle::ObjectHandle(void* object_, const DAVA::MetaInfo* objectType_)
    : object(object_)
    , objectType(objectType_)
{
    DVASSERT(object != nullptr);
    DVASSERT(objectType != nullptr);
}

ObjectHandle::ObjectHandle(DAVA::InspBase* object_)
{
    DVASSERT(object_ != nullptr);
    object = object_;
    objectType = object_->GetTypeInfo()->Type();
}

bool ObjectHandle::IsValid() const
{
    return object != nullptr && objectType != nullptr;
}

const DAVA::InspInfo* ObjectHandle::GetIntrospection() const
{
    return objectType->GetIntrospection(object);
}
}

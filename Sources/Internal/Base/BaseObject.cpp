#include "Base/BaseObject.h"
#include "Base/ObjectFactory.h"
#include "FileSystem/KeyedArchive.h"
#include "Base/Introspection.h"
#include "Utils/StringFormat.h"
#include "Reflection/ReflectionRegistrator.h"

namespace DAVA
{
DAVA_VIRTUAL_REFLECTION_IMPL(BaseObject)
{
    ReflectionRegistrator<BaseObject>::Begin()
    .Method("Release", &BaseObject::Release)
    .End();
}

const String& BaseObject::GetClassName() const
{
    return ObjectFactory::Instance()->GetName(this);
}

void BaseObject::SaveObject(KeyedArchive* archive)
{
    archive->SetString("##name", GetClassName());
}

BaseObject* BaseObject::LoadFromArchive(KeyedArchive* archive)
{
    String name = archive->GetString("##name");
    BaseObject* object = ObjectFactory::Instance()->New<BaseObject>(name);
    if (object)
    {
        object->LoadObject(archive);
    }
    return object;
}

void BaseObject::LoadObject(KeyedArchive* archive)
{
}
};

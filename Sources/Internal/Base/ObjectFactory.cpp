#include "Base/ObjectFactory.h"
#include "Logger/Logger.h"

namespace DAVA
{
ObjectFactory::ObjectFactory()
    : unregisteredClassName("<Unknown class>")
{
}

ObjectRegistrator::ObjectRegistrator(const String& name, CreateObjectFunc func, const std::type_info& info, uint32 size)
{
    ObjectFactory::Instance()->RegisterObjectCreator(name, func, info, size);
}

ObjectRegistrator::ObjectRegistrator(const String& name, CreateObjectFunc func, const std::type_info& info, uint32 size, const String& alias)
{
    ObjectFactory::Instance()->RegisterObjectCreator(name, func, info, size, alias);
}

void ObjectFactory::RegisterObjectCreator(const String& name, CreateObjectFunc func, const std::type_info& info, uint32 size)
{
    creatorMap[name] = func;
    nameMap[info.name()] = name;
    sizeMap[name] = size;
}

void ObjectFactory::RegisterObjectCreator(const String& name, CreateObjectFunc func, const std::type_info& info, uint32 size, const String& alias)
{
    creatorMap[alias] = func;
    nameMap[info.name()] = alias;
    sizeMap[alias] = size;
}

void ObjectFactory::Dump()
{
    Map<String, CreateObjectFunc>::iterator it = creatorMap.begin();
    for (; it != creatorMap.end(); ++it)
    {
        Logger::FrameworkDebug("Class: %s size: %d", it->first.c_str(), sizeMap[it->first]);
    }
}
}

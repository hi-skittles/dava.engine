#include "DAVAConfig.h"
#include "Base/BaseObject.h"
#include "Base/BaseObjectChecker.h"
#include "Debug/List.h"
#include "Logger/Logger.h"

#ifdef ENABLE_BASE_OBJECT_CHECKS

#include <set>

namespace DAVA
{
std::set<BaseObject*> baseObjects;

BaseObjectChecker::BaseObjectChecker
{
}

BaseObjectChecker::~BaseObjectChecker
{
}

void BaseObjectChecker::RegisterBaseObject(BaseObject* obj)
{
    baseObjects.insert(obj);
}

void BaseObjectChecker::UnregisterBaseObject(BaseObject* obj)
{
    std::set<BaseObject*>::iterator r = baseObjects.find(obj);
    if (r != baseObjects.end())
    {
        baseObjects.erase(r);
    }
}

bool BaseObjectChecker::IsAvailable(BaseObject* obj)
{
    std::set<BaseObject*>::iterator r = baseObjects.find(obj);
    if (r != baseObjects.end())
    {
        return true;
    }
    return false;
}

void BaseObjectChecker::Dump()
{
    for (std::set<BaseObject*>::iterator it = baseObjects.begin(); it != baseObjects.end(); ++it)
    {
        BaseObject* obj = *it;
        Logger::FrameworkDebug("(%s) object not released", typeid(obj).name());
    }
}
}

#endif // ENABLE_BASE_OBJECT_CHECKS

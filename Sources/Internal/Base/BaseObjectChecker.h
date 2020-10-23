#ifndef __DAVAENGINE_BASE_OBJECT_CHECKER_H__
#define __DAVAENGINE_BASE_OBJECT_CHECKER_H__

#include "Base/BaseTypes.h"
#include "Base/Singleton.h"
#include "DAVAConfig.h"

namespace DAVA
{
class BaseObject;
class BaseObjectChecker
{
public:
    BaseObjectChecker();
    virtual ~BaseObjectChecker();

    static void RegisterBaseObject(BaseObject* obj);
    static void UnregisterBaseObject(BaseObject* obj);
    static bool IsAvailable(BaseObject* obj);
    static void Dump();
};
}; 
#endif // __DAVAENGINE_BASE_OBJECT_CHECKER_H__

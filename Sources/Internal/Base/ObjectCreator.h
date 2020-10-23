#ifndef __DAVAENGINE_OBJECT_CREATOR_H__
#define __DAVAENGINE_OBJECT_CREATOR_H__

#include "Base/BaseTypes.h"
#include "Base/ObjectFactory.h"

namespace DAVA
{
/**
	\ingroup baseobjects
	\brief This is helper class to perform creation of objects 
 */
class ObjectCreator
{
public:
    ObjectCreator(const String& _name)
        : name(_name)
    {
    }

    virtual ~ObjectCreator() = default;

    virtual BaseObject* New()
    {
        return nullptr;
    }

    String name;
};

/**
	\ingroup baseobjects
	\brief This is helper class to perform creation of objects 
 */
template <class T>
class ObjectCreatorImpl : public ObjectCreator
{
public:
    ObjectCreatorImpl(const String& _name)
    {
        name = _name;
    }
    virtual BaseObject* New()
    {
        T* object = new T();
        return object;
    };
};
};

#endif // __DAVAENGINE_OBJECT_CREATOR_H__
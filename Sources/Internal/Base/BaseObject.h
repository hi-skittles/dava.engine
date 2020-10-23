#ifndef __DAVAENGINE_BASEOBJECT_H__
#define __DAVAENGINE_BASEOBJECT_H__

#include "Concurrency/Atomic.h"
#include "Base/BaseTypes.h"
#include "Base/BaseObjectChecker.h"
#include "Base/Introspection.h"
#include "Debug/DVAssert.h"
#include "DAVAConfig.h"
#include "Base/RefPtr.h"
#include "Base/ScopedPtr.h"
#include "Reflection/Reflection.h"

#include "MemoryManager/MemoryProfiler.h"

#include <typeinfo>
#include <memory>

namespace DAVA
{
/**
	\defgroup baseobjects Framework Base Objects
	This group contain all framework classes which defines the basics of our system. 
	All architecture approaches & patterns we've selected in our framework is well-known and described. 
 */
/** 
	\ingroup baseobjects
	\brief class to implement object reference counting
 
	This class if base object for most of hierarchies in our SDK. It's main purpose to help you avoid issues with memory, and provide you 
    with many high-level mechanisms like serialization, messaging and so on. In most cases if you create own class it will be good idea 
    to derive it from BaseObject. 
  */

class KeyedArchive;

class BaseObject : public InspBase
{
    DAVA_VIRTUAL_REFLECTION(BaseObject, InspBase);
    DAVA_ENABLE_CLASS_ALLOCATION_TRACKING(ALLOC_POOL_BASEOBJECT)

protected:
    //! Destructor
    virtual ~BaseObject()
    {
        DVASSERT(referenceCount == 0);
#ifdef ENABLE_BASE_OBJECT_CHECKS
        BaseObjectChecker::UnregisterBaseObject(this);
#endif
    }

public:
    //! Constructor
    BaseObject()
        : referenceCount(1)
    {
#ifdef ENABLE_BASE_OBJECT_CHECKS
        BaseObjectChecker::RegisterBaseObject(this);
#endif
    }

    /**
		\brief Increment reference counter in this object.
	 */
    virtual void Retain()
    {
        referenceCount++;
    }

    /** 
		\brief Decrement object reference counter and delete it if reference counter equal to 0.
		\returns referenceCounter value after decrement
	 */
    virtual int32 Release()
    {
#ifdef ENABLE_BASE_OBJECT_CHECKS
        if (!BaseObjectChecker::IsAvailable(this))
        {
            DVASSERT(0 && "Attempt to delete unavailable BaseObject");
        }	
#endif

        int32 refCounter = --referenceCount;
        if (!refCounter)
        {
            delete this;
        }
        return refCounter;
    }

    /** 
		\brief return current number of references for this object
		\returns referenceCounter value 
	 */
    int32 GetRetainCount() const
    {
        return referenceCount.Get();
    }

    /**
        \brief return class name if it's registered with REGISTER_CLASS macro of our ObjectFactory class.
        This function is mostly intended for serialization, but can be used for other purposes as well.
        \returns name of the class you've passed to REGISTER_CLASS function. For example if you register class UIButton with the following line:
        REGISTER_CLASS(UIButton); you'll get "UIButton" as result.
     */
    const String& GetClassName() const;

    virtual void SaveObject(KeyedArchive* archive);
    virtual void LoadObject(KeyedArchive* archive);

    static BaseObject* LoadFromArchive(KeyedArchive* archive);

    static BaseObject* DummyGet()
    {
        return 0;
    }

protected:
    BaseObject(const BaseObject& /*b*/)
    {
    }

    BaseObject& operator=(const BaseObject& /*b*/)
    {
        return *this;
    }

    Atomic<int32> referenceCount;
};

template <typename T>
auto MakeSharedObject(T* obj) -> typename std::enable_if<std::is_base_of<BaseObject, T>::value, std::shared_ptr<T>>::type
{
    DVASSERT(nullptr != obj);
    obj->Retain();
    return std::shared_ptr<T>(obj, [](T* obj) { obj->Release(); });
}

/** 
	\ingroup baseobjects
	\brief	function to perform release safely. It checks if given object not equal to zero, in debug mode it also checks if such object
			haven't deallocated before and only if both checks is positive it call Release. After release it set value of variable to 0 to avoid 
			possible errors with usage of this variable
 */
template <class C>
void SafeRelease(C*& c)
{
    if (c)
    {
#ifdef ENABLE_BASE_OBJECT_CHECKS
        if (!BaseObjectChecker::IsAvailable(c))
        {
            DVASSERT(0 && "SafeRelease Attempt to access unavailable BaseObject");
        }
#endif
        c->Release();
        c = nullptr;
    }
}

// /*#if defined(__DAVAENGINE_DIRECTX9__)*/
//template<>
//inline void SafeRelease<IUnknown>(IUnknown * &c)
//{
//	if (c)
//	{
//		c->Release();
//		c = 0;
//	}
//}
// // #endif
/** 
	\ingroup baseobjects
	\brief	function to perform retain safely. Only if object exists it perform retain.
	\return same object with incremented reference count
*/
template <class C>
C* SafeRetain(C* c)
{
    if (c)
    {
#ifdef ENABLE_BASE_OBJECT_CHECKS
        BaseObject* c2 = dynamic_cast<BaseObject*>(c);
        if (c2)
        {
            if (!BaseObjectChecker::IsAvailable(c))
            {
                DVASSERT(0 && "RetainedObject Attempt to access unavailable BaseObject");
            }
        }
#endif
        c->Retain();
    }
    return c;
}

using CreateObjectFunc = void* (*)();

class ObjectRegistrator
{
public:
    ObjectRegistrator(const String& name, CreateObjectFunc func, const std::type_info& typeinfo, uint32 size);
    ObjectRegistrator(const String& name, CreateObjectFunc func, const std::type_info& typeinfo, uint32 size, const String& alias);
};
};

#define REGISTER_CLASS(class_name) REGISTER_CLASS_WITH_NAMESPACE(class_name, )

#define REGISTER_CLASS_WITH_NAMESPACE(class_name, namespace_name) \
static void* Create##class_name()\
{\
return new namespace_name class_name();\
};\
static DAVA::ObjectRegistrator registrator##class_name(#class_name, &Create##class_name, typeid(namespace_name class_name), sizeof(namespace_name class_name));

#define REGISTER_CLASS_WITH_ALIAS(class_name, alias) REGISTER_CLASS_WITH_ALIAS_AND_NAMESPACE(class_name, alias, )

#define REGISTER_CLASS_WITH_ALIAS_AND_NAMESPACE(class_name, alias, namespace_name) \
static void* Create##class_name()\
{\
return new namespace_name class_name();\
};\
static DAVA::ObjectRegistrator registrator##class_name(#class_name, &Create##class_name, typeid(namespace_name class_name), sizeof(namespace_name class_name), alias);

#endif // __DAVAENGINE_BASEOBJECT_H__

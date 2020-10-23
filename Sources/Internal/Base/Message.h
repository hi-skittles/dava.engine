#ifndef __DAVAENGINE_MESSAGE__
#define __DAVAENGINE_MESSAGE__

#include "Base/BaseObject.h"

namespace DAVA
{
class MessageBase : public BaseObject
{
protected:
    virtual ~MessageBase();

public:
    virtual void operator()(BaseObject*, void*, void*) const = 0;
    virtual MessageBase* Clone() const = 0;
    virtual bool IsEqual(const MessageBase* message) const = 0;
    virtual BaseObject* GetBaseObject() const = 0;
};

template <class T, bool>
struct GetBaseObjectImpl
{
    BaseObject* Get(T* obj)
    {
        return nullptr;
    }
};

template <class T>
struct GetBaseObjectImpl<T, true>
{
    BaseObject* Get(T* obj)
    {
        return static_cast<BaseObject*>(obj);
    }
};

template <class T>
class MessageBaseClassFunctionImpl : public MessageBase
{
    T* targetObject;
    void (T::*targetFunction)(BaseObject*, void*, void*);

protected:
    ~MessageBaseClassFunctionImpl()
    {
    }

public:
    MessageBaseClassFunctionImpl(T* _object, void (T::*_function)(BaseObject*, void*, void*))
    {
        targetObject = _object;
        targetFunction = _function;
    }

    virtual void operator()(BaseObject* callerObject, void* userData, void* callerData) const
    {
        (targetObject->*targetFunction)(callerObject, userData, callerData);
    }

    virtual MessageBase* Clone() const
    {
        return new MessageBaseClassFunctionImpl(targetObject, targetFunction);
    }

    virtual bool IsEqual(const MessageBase* messageBase) const
    {
        const MessageBaseClassFunctionImpl<T>* t = dynamic_cast<const MessageBaseClassFunctionImpl<T>*>(messageBase);
        if (t != 0)
        {
            if (targetObject == t->targetObject && targetFunction == t->targetFunction)
                return true;
        }
        return false;
    }

    virtual BaseObject* GetBaseObject() const
    {
        return GetBaseObjectImpl<T, std::is_base_of<BaseObject, T>::value>().Get(targetObject);
    }
};

class MessageBaseStaticFunctionImpl : public MessageBase
{
    void (*targetFunction)(BaseObject*, void*, void*);

public:
    MessageBaseStaticFunctionImpl(void (*_function)(BaseObject*, void*, void*))
    {
        targetFunction = _function;
    }

    virtual void operator()(BaseObject* callerObject, void* userData, void* callerData) const
    {
        (*targetFunction)(callerObject, userData, callerData);
    }

    virtual MessageBase* Clone() const
    {
        return new MessageBaseStaticFunctionImpl(targetFunction);
    }

    virtual bool IsEqual(const MessageBase* messageBase) const
    {
        const MessageBaseStaticFunctionImpl* t = dynamic_cast<const MessageBaseStaticFunctionImpl*>(messageBase);
        if (t != 0)
        {
            if (targetFunction == t->targetFunction)
                return true;
        }
        return false;
    }

    virtual BaseObject* GetBaseObject() const
    {
        return 0;
    }
};

/**
	\ingroup baseobjects
	\brief	Message class defines a wrapper to store both pointer to class and it's function to call it easily. 
			All SDK subsystems use messages for event handling. 
 
	This code assign a message Message(this, &MenuScreen::ButtonPressed) to button event. 
 	\code
	playButton->AddEvent(UIControl::EVENT_TOUCH_UP_INSIDE, Message(this, &MenuScreen::ButtonPressed));
	\endcode
	This means that if event will be performed function ButtonPressed from MenuScreen 
	class with pointer to this will be called.
	This method is normally used together with \ref EventDispatcher class.
 */

class Message
{
    MessageBase* messageBase;
    void* userData;

public:
    template <class C>
    Message(C* targetObject, void (C::*targetFunction)(BaseObject*, void*, void*), void* _userData = 0)
    {
        userData = _userData;
        messageBase = new MessageBaseClassFunctionImpl<C>(targetObject, targetFunction);
    }

    Message(void (*targetFunction)(BaseObject*, void*, void*), void* _userData = 0)
    {
        userData = _userData;
        messageBase = new MessageBaseStaticFunctionImpl(targetFunction);
    }

    Message()
        : messageBase(0)
        , userData(0)
    {
    }

    ~Message()
    {
        SafeRelease(messageBase);
    }

    // void SetSelector(BaseObject *_pObj, void (BaseObject::*_pFunc)(BaseObject*, void*, void*), void * _pUserData);

    void operator()(BaseObject* caller) const
    {
        if (messageBase)
        {
            (*messageBase)(caller, userData, 0);
        }
    }

    void operator()(BaseObject* caller, void* callerData) const
    {
        if (messageBase)
        {
            (*messageBase)(caller, userData, callerData);
        }
    }

    bool operator==(const Message& msg) const
    {
        return (messageBase->IsEqual(msg.messageBase));
    }

    Message(const Message& msg)
    {
        messageBase = SafeRetain(msg.messageBase);
        userData = msg.userData;
    }

    Message& operator=(const Message& msg)
    {
        SafeRelease(messageBase);
        messageBase = SafeRetain(msg.messageBase);
        userData = msg.userData;
        return *this;
    }

    void SetUserData(void* newData)
    {
        userData = newData;
    }

    bool IsEmpty() const
    {
        return (messageBase == 0);
    }

    BaseObject* GetBaseObject() const
    {
        if (messageBase)
        {
            return messageBase->GetBaseObject();
        }

        return 0;
    }

    bool IsEqualWithUserdata(const Message& msg)
    {
        return (messageBase->IsEqual(msg.messageBase) && userData == msg.userData);
    }
};
};

#endif
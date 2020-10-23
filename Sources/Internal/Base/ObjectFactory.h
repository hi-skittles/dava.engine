#ifndef __DAVAENGINE_OBJECT_FACTORY_H__
#define __DAVAENGINE_OBJECT_FACTORY_H__

#include "Base/BaseTypes.h"
#include "Base/BaseObject.h"
#include "Base/StaticSingleton.h"
#include "Base/ObjectCreator.h"
#include "Utils/StringFormat.h"
#include <typeinfo>

namespace DAVA
{
/*#define REGISTER_CLASS_WITH_STRING_NAME(class_name, symbol_name) \
static BaseObject * Create##class_name()\
{\
return new class_name();\
};\
static ObjectRegistrator registrator##class_name(symbol_name, &Create##class_name);
*/

/**
	\ingroup baseobjects
	\brief	This class allow to create classes by name
 
	For example you want to create bunch of classes using their names it can be done easily using framework
	In implementation of your class you should write REGISTER_CLASS(ClassName) and this class now can 
	be created by it's name using ObjectFactory singleton
	
	Class must be inherited from BaseObject and have default constructor.
 
	Example:
	
	Definition of class
	\code
	// UIButton.h
	class UIButton : public BaseObject
	{
		UIButton::UIButton()
 
	};
	\endcode
	
	Implementation of class
	\code
	// UIButton.cpp
	
	REGISTER_CLASS(UIButton);
	
	UIButton::UIButton()
	{
	}
	\endcode
		
	Usage of ObjectFactory:
	\code
		
	void MyFunction()
	{
		UIButton * button = dynamic_cast<UIButton*>(ObjectFactory::Instance()->New("UIButton"));
		
	}
 
	\endcode 
 */

class ObjectFactory : public StaticSingleton<ObjectFactory>
{
public:
    ObjectFactory();

    /**
     \brief check an ability to create class with given name
     
     \param[in] name name of class you want to create
     */
    bool IsTypeRegistered(const String& name) const
    {
        return (creatorMap.find(name) != creatorMap.end());
    }

    /**
		\brief creates a class with given name
		
		\param[in] name name of class you want to create
	 */

    template <class T>
    T* New(const String& name)
    {
        Map<String, CreateObjectFunc>::iterator it = creatorMap.find(name);
        if (it != creatorMap.end())
        {
            CreateObjectFunc newFunc = it->second;

            //VI: cannot use CastIfEqual since we need to cast to base types
            //VI: but CastIfEqual casts to the exact types only
            return static_cast<T*>((newFunc)());
        }
        DVASSERT(false, Format("Class %s creator not found.", name.c_str()).c_str());
        return 0;
    }

    /**
        \brief creates a class with given name

        \param[in] name name of class you want to create
        \param[in] object you can pass as creation parameter
	 */
    template <class S, class T>
    T* New(const String& name, const S& object);

    //    /**
    //        \brief
    //     */
    //    template <class T>
    //    T * New(const String & name);

    template <class T>
    const String& GetName(T* t);

    /**
		\brief This function is supposed to RegisterObjectCreator
		
		It used internally by REGISTER_CLASS define and REGISTER_CLASS_WITH_ALIAS define.
        Both defines are in BaseObject.h
     
		\param[in] name this is name of class we want to register
		\param[in] func this is pointer to function that can create such class
        \param[in] alias this name can be used if you want to save object as his parent
	*/
    void RegisterObjectCreator(const String& name, CreateObjectFunc func, const std::type_info& typeinfo, uint32 size);
    void RegisterObjectCreator(const String& name, CreateObjectFunc func, const std::type_info& typeinfo, uint32 size, const String& alias);

    void Dump();

private:
    Map<String, CreateObjectFunc> creatorMap;
    Map<String, String> nameMap;
    Map<String, uint32> sizeMap;
    String unregisteredClassName;
};

template <class T>
const String& ObjectFactory::GetName(T* t)
{
    Map<String, String>::iterator it = nameMap.find(typeid(*t).name());
    if (it != nameMap.end())
    {
        return it->second;
    }
    return unregisteredClassName;
}
};

#endif // __DAVAENGINE_OBJECT_FACTORY_H__

#ifndef __DAVAENGINE_INTROSPECTION_BASE_H__
#define __DAVAENGINE_INTROSPECTION_BASE_H__

#include "Base/BaseTypes.h"
#include "Base/FastName.h"
#include "Base/GlobalEnum.h"
#include "Base/TemplateHelpers.h"
#include "FileSystem/VariantType.h"
#include "Reflection/Reflection.h"

namespace DAVA
{
class InspInfo;
class InspMemberDynamic;
class InspColl;
class KeyedArchive;
struct MetaInfo;

// абстрактный базовый класс для интроспекции
class InspBase : public ReflectionBase
{
    DAVA_VIRTUAL_REFLECTION(InspBase, ReflectionBase);

public:
    InspBase();

    // Возвращает интроспекцию класса
    virtual const InspInfo* GetTypeInfo() const
    {
        return nullptr;
    };

protected:
    virtual ~InspBase();
};

struct InspDesc
{
    enum Type
    {
        T_UNDEFINED,
        T_ENUM,
        T_FLAGS,
    };

    const char* text = "";
    const EnumMap* enumMap = nullptr;
    Type type = T_UNDEFINED;

    InspDesc()
    {
    }

    InspDesc(const char* _text)
        : text(_text)
    {
    }

    explicit InspDesc(const char* _text, const EnumMap* _enumMap, InspDesc::Type _type = T_ENUM)
        : text(_text)
        , enumMap(_enumMap)
        , type(_type)
    {
    }

    InspDesc& operator=(const InspDesc& desc)
    {
        text = desc.text;
        enumMap = desc.enumMap;
        type = desc.type;

        return *this;
    }
};

// Базовое представление члена интроспекции
class InspMember
{
    friend class InspInfo;

public:
    InspMember(const char* _name, const InspDesc& _desc, const size_t _offset, const MetaInfo* _type, int _flags = 0);
    virtual ~InspMember() = default;

    // Имя члена интроспекции, соответствует имени члена класса
    const FastName& Name() const;

    // Описание члена интроспекции, произвольно указанное пользователем при объявлении интроспекции
    const InspDesc& Desc() const;

    // Возвдащает мета-тип члена интроспекции
    const MetaInfo* Type() const;

    // Возвращает указатель непосдедственно на данные члена интроспекции
    // Следует учитывать, что если членом интроспекции является указатель,
    // то данная функция вернет указатель на указатель
    // Проверить является ли член интроспекции указателем можно по его мета-информации:
    //   MetaInfo* meta = member->Type();
    //   meta->IsPointer();
    //
    // для int  a    - функция вернет &a, тип int *
    // для int* a    - функция вернет &a, тип int **
    // для classC  c - функция вернет &c, тип c *
    // для classC* c - функция вернет &c, тип с **
    //
    // см. так же функцию Object()
    //
    virtual void* Pointer(void* object) const;

    // Вернет указатель на объек данного члена интроспекции.
    // Функция сама проверит, является ли данный член интроспекции указателем и
    // если он таковым является то вернет значение данного указателя.
    // Привер по типам члена интроспекции:
    //
    // для int  a    - функция вернет &a, тип int *
    // для int* a    - функция вернет a,  тип int *
    // для classC  c - функция вернет &c, тип classC *
    // для classC* c - функция вернет c,  тип classC *
    //
    virtual void* Data(void* object) const;

    virtual VariantType::eVariantType ValueType() const;
    // Возвращает вариант данных члена интроспекции. Имлементация варианта должна поддерживать
    // создание из данных, определяемыт мета-типом данного члена интроспекции.
    virtual VariantType Value(void* object) const;

    // Устанавливает данные члена интроспекции из указанного варианта.
    virtual void SetValue(void* object, const VariantType& val) const;

    virtual void SetValueRaw(void* object, void* val) const;

    // Возвращает данные члена интроспекции в виде коллекции
    virtual const InspColl* Collection() const;

    virtual const InspMemberDynamic* Dynamic() const;

    const InspInfo* GetParentInsp() const;

    int Flags() const;

protected:
    void ApplyParentInsp(const InspInfo* parentInsp) const;

    FastName name;
    InspDesc desc;
    const size_t offset;
    const MetaInfo* type;
    const int flags;
    mutable const InspInfo* parentInsp;
};

// Базовое представление члена интроспекции, являющегося коллекцией
class InspColl : public InspMember
{
public:
    using Iterator = void*;

    InspColl(const char* _name, const InspDesc& _desc, const size_t _offset, const MetaInfo* _type, int _flags = 0);

    virtual MetaInfo* CollectionType() const = 0;
    virtual MetaInfo* ItemType() const = 0;
    virtual int Size(void* object) const = 0;
    virtual Iterator Begin(void* object) const = 0;
    virtual Iterator Next(Iterator i) const = 0;
    virtual void Finish(Iterator i) const = 0;
    virtual void ItemValueGet(Iterator i, void* itemDst) const = 0;
    virtual void ItemValueSet(Iterator i, void* itemSrc) const = 0;
    virtual void* ItemPointer(Iterator i) const = 0;
    virtual void* ItemData(Iterator i) const = 0;
    virtual MetaInfo* ItemKeyType() const = 0;
    virtual const void* ItemKeyPointer(Iterator i) const = 0;
    virtual const void* ItemKeyData(Iterator i) const = 0;
};

// Вспомогательный класс для определения содержит ли указанный шаблонный тип интроспекцию
// Наличие интроспекции определяется по наличию функции GetTypeInfo у данного типа
template <typename T>
struct HasInspImpl
{
protected:
    class yes
    {
        char m;
    };
    class no
    {
        yes m[2];
    };

    // Базовый класс для проверки, содержит пустую искомую функцию
    struct TestBase
    {
        const InspInfo* GetTypeInfo();
    };

    // Для проверки типа Т мы создаем склаа наследованный от TestBase и Т
    // Таким образом если Т содержит функцию GetTypeInfo, то Test::GetTypeInfo будет указывать
    // на Т::GetTypeInfo, иначе на TestBase::GetTypeInfo
    struct Test : public T, public TestBase
    {
    };

    // Вспомогательный класс для вывода типов
    template <typename C, C c>
    struct Helper
    {
    };

    // Функция для проверки является содержит заданный тип U имплементацию функции TestBase::GetTypeInfo
    // Компилятор сможет вывести типы для класса Helper только в том случае если &U::GetTypeInfo соответствует
    // указателю на функцию GetTypeInfo, являющуюся членом класса TestBase
    template <typename U>
    static no Check(U*, Helper<const InspInfo* (TestBase::*)(), &U::GetTypeInfo>* = nullptr);

    // В случае когда вывод типов невозможен для первой функции, будет вызвана эта. Это произойдет только тогда,
    // когда Т содержит свою функцию GetTypeInfo, а следовательно содержит интроспекцию
    static yes Check(...);

public:
    // Статическая переменна, значение которой будет равно true в случае,
    // когда тип Т содержит интроспекцию
    static const bool result = (sizeof(yes) == sizeof(Check(static_cast<Test*>(nullptr))));
};

// Параметризированные имплементации HasIntrospection для базовых типов
// (так как наследование класса Test от базового типа невозможно)
template <>
struct HasInspImpl<void>
{
    static const bool result = false;
};
template <>
struct HasInspImpl<bool>
{
    static const bool result = false;
};
template <>
struct HasInspImpl<char8>
{
    static const bool result = false;
};
template <>
struct HasInspImpl<char16>
{
    static const bool result = false;
};
template <>
struct HasInspImpl<int8>
{
    static const bool result = false;
};
template <>
struct HasInspImpl<uint8>
{
    static const bool result = false;
};
template <>
struct HasInspImpl<int16>
{
    static const bool result = false;
};
template <>
struct HasInspImpl<uint16>
{
    static const bool result = false;
};
template <>
struct HasInspImpl<int32>
{
    static const bool result = false;
};
template <>
struct HasInspImpl<uint32>
{
    static const bool result = false;
};
template <>
struct HasInspImpl<int64>
{
    static const bool result = false;
};
template <>
struct HasInspImpl<uint64>
{
    static const bool result = false;
};
template <>
struct HasInspImpl<float32>
{
    static const bool result = false;
};
template <>
struct HasInspImpl<float64>
{
    static const bool result = false;
};
template <>
struct HasInspImpl<KeyedArchive*>
{
    static const bool result = false;
};

template <typename T>
struct HasNotInsp
{
    static const bool result = false;
};

template <typename T>
struct HasInsp
{
    using CheckInspImpl = typename Select<IsEnum<T>::result /* || IsUnion<T>::result */, HasNotInsp<T>, HasInspImpl<T>>::Result;
    enum
    {
        result = CheckInspImpl::result
    };
};

// Глобальная шаблонная функция(#1) для получения интроспекции заданного типа
// Функция скомпилируется только для тех типов, для которых HasIntrospection<T>::result будет true
template <typename T>
typename EnableIf<HasInsp<T>::result, const InspInfo*>::type GetIntrospection()
{
    return T::TypeInfo();
}

// Глобальная шаблонная функция(#2) для получения интроспекции заданного типа
// Функция скомпилируется только для тех типов, для которых HasIntrospection<T>::result будет false
template <typename T>
typename EnableIf<!HasInsp<T>::result, const InspInfo*>::type GetIntrospection()
{
    return nullptr;
}

// Глобальная шаблонная функция(#3) для получения интроспекции типа заданного объекта.
// Тип объекта будет выведен компилятором автоматически.
// Функция скомпилируется только для тех типов, для которых HasIntrospection<T>::result будет true
// Пример:
//
// class A {};
// class B : public class A { ... INTROSPECTION ... }
// B *b;
// GetIntrospection(b)			// вернет интроспекцию класса B
// GetIntrospection((A *) b)	// вернет nullptr, т.к. будет вызвана функция #4, см. ниже
//
template <typename T>
typename EnableIf<HasInsp<T>::result, const InspInfo*>::type GetIntrospection(const T* t)
{
    const InspInfo* ret = nullptr;

    if (nullptr != t)
    {
        ret = t->GetTypeInfo();
    }

    return ret;
}

// Глобальная шаблонная функция(#4) для получения интроспекции типа заданного объекта.
// Тип объекта будет выведен компилятором автоматически.
// Функция скомпилируется только для тех типов, для которых HasIntrospection<T>::result будет false
template <typename T>
typename EnableIf<!HasInsp<T>::result, const InspInfo*>::type GetIntrospection(const T* t)
{
    return nullptr;
}

template <typename T>
const InspInfo* GetIntrospectionByObject(void* object)
{
    const T* t = static_cast<const T*>(object);
    return GetIntrospection(t);
}

/*
	template<typename T> 
	struct HasIntrospection
	{
		template<int N>
		struct CheckHelper
		{
			char x[N];
		};

		template<typename Q>
		static inline char Check(Q *t, CheckHelper<sizeof(&Q::GetTypeInfo)> *u)
		{
			return sizeof(*u);
		}

		static inline int Check(...)
		{
			return 4;
		}

		static const bool result = (1 == sizeof(Check((T *) 0)));
	};


	template<typename T, bool hasIntrospection> 
	struct GetIntrospectionBase;

	template<typename T> 
	struct GetIntrospectionBase<T, false>
	{
		static inline const IntrospectionInfo* GetInfo() { return nullptr; }
	};

	template<typename T> 
	struct GetIntrospectionBase<T, true>
	{
		static inline const IntrospectionInfo* GetInfo() { return &T::GetTypeInfo(); }
	};

	// Глобальная шаблонная функция(#2) для получения интроспекции заданного типа
	// Функция скомпилируется только для тех типов, для которых HasIntrospection<T>::result будет false
	template<typename T>
	const IntrospectionInfo* GetIntrospection() 
	{
		return GetIntrospectionBase<T, HasIntrospection<T>::result>::GetInfo();
	}

	template<typename T>
	const IntrospectionInfo* GetIntrospection(const T *t) 
	{
		return GetIntrospectionBase<T, HasIntrospection<T>::result>::GetInfo();
	}

	*/
};

#endif // __DAVAENGINE_INTROSPECTION_BASE_H__

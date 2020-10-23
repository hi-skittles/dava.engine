#ifndef __DAVAENGINE_INTROSPECTION_H__
#define __DAVAENGINE_INTROSPECTION_H__

#include "Debug/DVAssert.h"
#include "FileSystem/VariantType.h"

#include "Base/Meta.h"
#include "Base/IntrospectionBase.h"
#include "Base/IntrospectionProperty.h"
#include "Base/IntrospectionCollection.h"
#include "Base/IntrospectionFlags.h"
#include "Base/IntrospectionDynamic.h"
#include "Base/BaseTypes.h"

namespace DAVA
{
// Класс интроспекции.
// Добавляет интроспекцию к любому пользовательскому классу. Интроспекция статическая и существует в единичном экземпляре,
// не зависимо от количества объектов класса.
//
// Использование:
//
// 	class A
// 	{
// 	public:
// 		int		i;
// 		String	s;
// 		Matrix4	m;
//		Vector<int> v;
//
//		String GetName() { return s; }
//      void SetName(const String &_s) { s = _s; }
//
// 		INTROSPECTION(A,
// 			MEMBER(i, "simple int var", 0)
// 			MEMBER(s, "string", 0)
//			PROPERTY(s, "property with setter and getter", GetName, SetName, 0)
//			COLLECTION(v, "vector collection")
// 			);
// 	};
//
//  class B : public A
//	{
//	public:
//		int b;
//
//		INTROSPECTION_EXTEND(B, A,
//			MEMBER(b)
//			);
//	}
//
class InspInfo
{
public:
    InspInfo(const char* _name, const InspMember** _members, const int _members_count)
        : name(_name)
        , meta(nullptr)
        , base_info(nullptr)
        , members(_members)
        , members_count(_members_count)
    {
        MembersInit();
    }

    InspInfo(const InspInfo* _base, const char* _name, const InspMember** _members, const int _members_count)
        : name(_name)
        , meta(nullptr)
        , base_info(_base)
        , members(_members)
        , members_count(_members_count)
    {
        MembersInit();
    }

    ~InspInfo()
    {
        MembersRelease();
    }

    InspInfo(const InspInfo&) = delete;

    const FastName& Name() const
    {
        return name;
    }

    const MetaInfo* Type() const
    {
        return meta;
    }

    int MembersCount() const
    {
        return members_count;
    }

    // Возвращает указатель на член интроспекции по заданному индексу, или nullptr если такой не найден.
    const InspMember* Member(int index) const
    {
        const InspMember* member = nullptr;

        if (index < members_count)
            if (nullptr != members[index])
                member = members[index];

        return member;
    }

    // Возвращает указатель на член интроспекции по заданному имени, или nullptr если такой не найден.
    const InspMember* Member(const FastName& name) const
    {
        for (int i = 0; i < members_count; ++i)
        {
            if (nullptr != members[i])
            {
                if (members[i]->name == name)
                {
                    return members[i];
                }
            }
        }

        return nullptr;
    }

    // Возвращает указатель на базовую интроспекцию, или nullptr если такой не существует.
    const InspInfo* BaseInfo() const
    {
        return base_info;
    }

    // Единожды установить в текущей интроспекции для типа Т указатель
    // на мета-информацию типа T
    template <typename T>
    void OneTimeMetaSafeSet()
    {
        if (!metaOneTimeSet)
        {
            metaOneTimeSet = true;
            meta = MetaInfo::Instance<T>();
        }
    }

protected:
    FastName name;
    const MetaInfo* meta;

    const InspInfo* base_info;
    const InspMember** members;
    int members_count;

    bool metaOneTimeSet;

    // Инициализация членов интроспекции
    // Все члены интроспекция должны быть валидны(созданы), в противном случае
    // данная интроспекция будет пустой
    void MembersInit()
    {
        // Проверяем или все члены интроспекции валидны
        for (int i = 0; i < members_count; ++i)
        {
            // Если хоть один не создан, то освобождаем все остальные.
            if (nullptr == members[i])
            {
                MembersRelease();
                break;
            }

            // обратная связь члена интроспекции непостредственно к интроспекции
            members[i]->ApplyParentInsp(this);
        }
    }

    // Освобождает члены интроспекции и устанавливает их количество в 0
    void MembersRelease()
    {
        for (int i = 0; i < members_count; ++i)
        {
            if (nullptr != members[i])
            {
                delete members[i];
                members[i] = nullptr;
            }
        }
        members_count = 0;
    }
};

class InspInfoDynamic
{
    friend class InspMemberDynamic;

public:
    struct DynamicData
    {
        void* object = nullptr;
        std::shared_ptr<void> data;
    };

    InspInfoDynamic()
        : memberDynamic(NULL)
    {
    }
    virtual ~InspInfoDynamic()
    {
    }

    virtual DynamicData Prepare(void* object, int filter = 0) const = 0;
    virtual Vector<FastName> MembersList(const DynamicData& ddata) const = 0;
    virtual InspDesc MemberDesc(const DynamicData& ddata, const FastName& member) const = 0;
    virtual int MemberFlags(const DynamicData& ddata, const FastName& member) const = 0;
    virtual VariantType MemberAliasGet(const DynamicData& ddata, const FastName& member) const
    {
        return VariantType();
    }
    virtual VariantType MemberValueGet(const DynamicData& ddata, const FastName& member) const = 0;
    virtual void MemberValueSet(const DynamicData& ddata, const FastName& member, const VariantType& value) = 0;

    const InspMemberDynamic* GetMember() const
    {
        return memberDynamic;
    }

protected:
    const InspMemberDynamic* memberDynamic;
};
};

// Определение интоспекции внутри класса. См. пример в описании класса IntrospectionInfo
#define INTROSPECTION(_type, _members) \
	static const DAVA::InspInfo* TypeInfo() \
	{ \
		using ObjectT = _type; \
		static const DAVA::InspMember* data[] = { _members }; \
		static DAVA::InspInfo info(#_type, data, sizeof(data) / sizeof(data[0])); \
		info.OneTimeMetaSafeSet<ObjectT>(); \
        return &info; \
	} \
    const DAVA::InspInfo* GetTypeInfo() const override \
	{ \
		return _type::TypeInfo(); \
	}

// Наследование интоспекции. См. пример в описании класса IntrospectionInfo
#define INTROSPECTION_EXTEND(_type, _base_type, _members) \
	static const DAVA::InspInfo* TypeInfo() \
	{ \
		using ObjectT = _type; \
		static const DAVA::InspMember* data[] = { _members }; \
		static DAVA::InspInfo info(_base_type::TypeInfo(), #_type, data, sizeof(data) / sizeof(data[0])); \
		info.OneTimeMetaSafeSet<ObjectT>(); \
		return &info; \
	} \
    const DAVA::InspInfo* GetTypeInfo() const override \
	{ \
		return _type::TypeInfo(); \
	}

// Определение обычного члена интроспекции. Доступ к нему осуществляется непосредственно.
#define MEMBER(_name, _desc, _flags) \
	new DAVA::InspMember(#_name, _desc, reinterpret_cast<size_t>(&((static_cast<ObjectT*>(nullptr))->_name)), DAVA::MetaInfo::Instance(&ObjectT::_name), _flags),

// Определение члена интроспекции, как свойства. Доступ к нему осуществляется через функци Get/Set. 
#define PROPERTY(_name, _desc, _getter, _setter, _flags) \
	DAVA::CreateIspProp(_name, _desc, &ObjectT::_getter, &ObjectT::_setter, _flags),

// Определение члена интроспекции, как коллекции. Доступ - см. IntrospectionCollection
#define COLLECTION(_name, _desc, _flags) \
	DAVA::CreateInspColl(&static_cast<ObjectT*>(nullptr)->_name, #_name, _desc, reinterpret_cast<size_t>(&static_cast<ObjectT*>(nullptr)->_name), DAVA::MetaInfo::Instance(&ObjectT::_name), _flags),

// Определение члена интроспекции с динамической структурой. Структуру определяет _dynamic, импементирующая интерфейс InspDynamicInfo
#define DYNAMIC(_name, _desc, _dynamic, _flags) \
	new DAVA::InspMemberDynamic(#_name, _desc, DAVA::MetaInfo::Instance<void*>(), _flags, _dynamic),

#endif // __DAVAENGINE_INTROSPECTION_H__

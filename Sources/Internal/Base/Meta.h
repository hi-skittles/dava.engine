#ifndef __DAVAENGINE_META_H__
#define __DAVAENGINE_META_H__

#include "Base/IntrospectionBase.h"

namespace DAVA
{
// Класс мета-информации типов.
struct MetaInfo
{
    using IntrospectionInfoFnPtr = const InspInfo* (*)(void*);

    // MetaInfo::Instance<Т>() - возвращает постоянный для типа Т указатель на мета-информацию.
    template <typename MetaT>
    static MetaInfo* Instance()
    {
        // Для того, чтобы гарантировать постоянный и единственный указатель на мета-информацию для типа Т
        // эта МИ объявлена как статическая переменная.
        static MetaInfo metaInfo(typeid(MetaT).name(), sizeof(MetaT), PointerTraits<MetaT>::result);

        // Попробуем найти интроспекцию для данного типа Т и сохранить ее в данной мета-информации (МИ).
        // Почему поик интроспекции не сделан в конструкторе МИ? Потому, что в этом случае еще не завершившийся статический конструктор МИ
        // попытается получить инстанс интроспекци, которой в свою очередь, при создании, требуется инстанс данной МИ. Это приведет к повторному
        // еще не завершившегося конструктора МИ.
        metaInfo.OneTimeIntrospectionSafeSet<MetaT>();

        // Возвращаем инстанс мета-информации.
        return &metaInfo;
    }

    // MetaInfo::Instance<C, Т>(T C::*var) - принимает указатель на член класса С типа Т и возвращает
    // постоянный для Т указатель на мета-информацию.
    //
    // Данная функция-врапер используется для реализации Интроспекции.
    // Не рекомендуется ее использование напрямую.
    template <typename ClassT, typename MemberT>
    static MetaInfo* Instance(MemberT ClassT::*var)
    {
        return MetaInfo::Instance<MemberT>();
    }

    // Размер типа в байтах
    inline int GetSize() const
    {
        return type_size;
    }

    // Имя типа
    // Результат данной функции сильно зависит от платформы и компилятора и следовательно его
    // использование целесообразно ТОЛЬКО ПРИ ОТЛАДКИ.
    //
    // Абсолютно НЕВЕРНЫМ является сравнение типа:
    // ("int" == meta->GetTypeName())
    //
    // ПРАВИЛЬНЫЙ вариант:
    // (MetaInto::Instance<int>() == meta)
    inline const char* GetTypeName() const
    {
        return type_name;
    }

    // Возвращает true если данный тип является указателем
    inline bool IsPointer() const
    {
        return isPointer;
    }

    // Получение интроспекции для данного типа
    // В случае отсутствия интроспекции функциия вернет NULL
    //
    // ВАЖНО: Результат данной фунции будет идентичен для типа Т и Т*
    // Пользователь должен использовать функцию IsPointer() для разрешения
    inline const InspInfo* GetIntrospection() const
    {
        return introspection;
    }

    // Получение интроспекции для данного обекта
    // В случае отсутствия интроспекции функциия вернет NULL
    //
    // ВАЖНО: Если тип входящего объекта не соответствует типу данной мета-информации,
    // то результат выполнения данной функции НЕПРЕДСКАЗУЕМ
    inline const InspInfo* GetIntrospection(void* object) const
    {
        const InspInfo* intro = NULL;

        if (NULL != object && NULL != introspectionFnPtr)
        {
            intro = introspectionFnPtr(object);
        }

        return intro;
    }

protected:
    // Единожды установить в текущей мета-информации T указатель на интроспекцию
    // заданного типа IntrospectionT (IntrospectionT == T всегда)
    template <typename IntrospectionT>
    void OneTimeIntrospectionSafeSet()
    {
        if (!introspectionOneTimeSet)
        {
            introspectionOneTimeSet = true;

            // Как работает данный шаблон:
            // typename Select<res1, T1, T2>::Result
            // Если результат res1 является истиной, то выбирается тип T1 иначе T2
            //
            // т.е. <typename Select<PointerTraits<res1, T1, T2>::Result>,
            //
            // при res1 == true преобразуется в
            //
            // DAVA::GetIntrospection<typename T1>
            //
            // PointerTraits<T>::result - вернет true, если тип Т является указателем, а
            // PointerTraits<IntrospectionT>::PointeeType - вернет тип указателя
            introspection = DAVA::GetIntrospection<typename Select<PointerTraits<IntrospectionT>::result, typename PointerTraits<IntrospectionT>::PointerType, IntrospectionT>::Result>();

            // Получение указателя на функцию извлечения интроспекции из объекта
            introspectionFnPtr = &DAVA::GetIntrospectionByObject<typename Select<PointerTraits<IntrospectionT>::result, typename PointerTraits<IntrospectionT>::PointerType, IntrospectionT>::Result>;
        }
    }

private:
    MetaInfo(const char* _type_name, int _type_size, bool is_pointer)
        : type_size(_type_size)
        , type_name(_type_name)
        , introspection(NULL)
        , introspectionFnPtr(NULL)
        , introspectionOneTimeSet(false)
        , isPointer(is_pointer)
    {
    }

    const int type_size;
    const char* type_name;
    const InspInfo* introspection;
    IntrospectionInfoFnPtr introspectionFnPtr;

    bool introspectionOneTimeSet;
    bool isPointer;
};
};

#endif // __DAVAENGINE_META_H__

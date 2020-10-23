#ifndef __DAVAENGINE_INTROSPECTION_COLLECTION_H__
#define __DAVAENGINE_INTROSPECTION_COLLECTION_H__

#include "Base/IntrospectionBase.h"

namespace DAVA
{
// Класс представляет расширение базового класса IntrospectionMember и описывает члена интроспекции, как коллекцию
// Поддерживаемые коллекци - контейнеры с одним шаблонным параметром: Vector, List, Set
template <template <typename, typename> class C, typename T, typename A>
class InspCollImpl : public InspColl
{
public:
    using CollectionT = C<T, A>;

    InspCollImpl(const char* _name, const InspDesc& _desc, const size_t _offset, const MetaInfo* _type, int _flags = 0)
        : InspColl(_name, _desc, _offset, _type, _flags)
    {
    }

    DAVA::MetaInfo* CollectionType() const
    {
        return DAVA::MetaInfo::Instance<CollectionT>();
    }

    DAVA::MetaInfo* ItemType() const
    {
        return DAVA::MetaInfo::Instance<T>();
    }

    int Size(void* object) const
    {
        int size = 0;

        if (nullptr != object)
        {
            CollectionT* collection = static_cast<CollectionT*>(object);
            size = static_cast<int>(collection->size());
        }

        return size;
    }

    Iterator Begin(void* object) const
    {
        Iterator i = nullptr;

        if (nullptr != object)
        {
            CollectionT* collection = static_cast<CollectionT*>(object);

            typename CollectionT::iterator begin = collection->begin();
            typename CollectionT::iterator end = collection->end();

            if (begin != end)
            {
                CollectionPos* pos = new CollectionPos();
                pos->curPos = begin;
                pos->endPos = end;

                i = static_cast<Iterator>(pos);
            }
        }

        return i;
    }

    Iterator Next(Iterator i) const
    {
        CollectionPos* pos = static_cast<CollectionPos*>(i);

        if (nullptr != pos)
        {
            pos->curPos++;

            if (pos->curPos == pos->endPos)
            {
                delete pos;
                i = nullptr;
            }
        }

        return i;
    }

    void Finish(Iterator i) const
    {
        CollectionPos* pos = static_cast<CollectionPos*>(i);
        if (nullptr != pos)
        {
            delete pos;
        }
    }

    void ItemValueGet(Iterator i, void* itemDst) const
    {
        CollectionPos* pos = static_cast<CollectionPos*>(i);
        if (nullptr != pos)
        {
            T* dstT = static_cast<T*>(itemDst);
            *dstT = *(pos->curPos);
        }
    }

    void ItemValueSet(Iterator i, void* itemSrc) const
    {
        CollectionPos* pos = static_cast<CollectionPos*>(i);
        if (nullptr != pos)
        {
            T* srcT = static_cast<T*>(itemSrc);
            *(pos->curPos) = *srcT;
        }
    }

    void* ItemPointer(Iterator i) const
    {
        void* p = nullptr;
        CollectionPos* pos = static_cast<CollectionPos*>(i);

        if (nullptr != pos)
        {
            p = &(*(pos->curPos));
        }

        return p;
    }

    void* ItemData(Iterator i) const
    {
        if (ItemType()->IsPointer())
        {
            return *(static_cast<void**>(ItemPointer(i)));
        }
        else
        {
            return ItemPointer(i);
        }
    }

    MetaInfo* ItemKeyType() const
    {
        return nullptr;
    }

    const void* ItemKeyPointer(Iterator i) const
    {
        return nullptr;
    }

    const void* ItemKeyData(Iterator i) const
    {
        return nullptr;
    }

    const InspColl* Collection() const
    {
        return this;
    }

protected:
    struct CollectionPos
    {
        typename CollectionT::iterator curPos;
        typename CollectionT::iterator endPos;
    };
};

template <template <typename, typename, typename> class C, typename K, typename V, typename A>
class InspKeyedCollImpl : public InspColl
{
public:
    using CollectionT = C<K, V, A>;

    InspKeyedCollImpl(const char* _name, const InspDesc& _desc, const size_t _offset, const MetaInfo* _type, int _flags = 0)
        : InspColl(_name, _desc, _offset, _type, _flags)
    {
    }

    DAVA::MetaInfo* CollectionType() const
    {
        return DAVA::MetaInfo::Instance<CollectionT>();
    }

    DAVA::MetaInfo* ItemType() const
    {
        return DAVA::MetaInfo::Instance<V>();
    }

    int Size(void* object) const
    {
        int size = 0;

        if (nullptr != object)
        {
            CollectionT* collection = static_cast<CollectionT*>(object);
            size = collection->size();
        }

        return size;
    }

    Iterator Begin(void* object) const
    {
        Iterator i = nullptr;

        if (nullptr != object)
        {
            CollectionT* collection = static_cast<CollectionT*>(object);

            typename CollectionT::iterator begin = collection->begin();
            typename CollectionT::iterator end = collection->end();

            if (begin != end)
            {
                CollectionPos* pos = new CollectionPos();
                pos->curPos = begin;
                pos->endPos = end;

                i = static_cast<Iterator>(pos);
            }
        }

        return i;
    }

    Iterator Next(Iterator i) const
    {
        CollectionPos* pos = static_cast<CollectionPos*>(i);

        if (nullptr != pos)
        {
            pos->curPos++;

            if (pos->curPos == pos->endPos)
            {
                delete pos;
                i = nullptr;
            }
        }

        return i;
    }

    void Finish(Iterator i) const
    {
        CollectionPos* pos = static_cast<CollectionPos*>(i);
        if (nullptr != pos)
        {
            delete pos;
        }
    }

    void ItemValueGet(Iterator i, void* itemDst) const
    {
        CollectionPos* pos = static_cast<CollectionPos*>(i);
        if (nullptr != pos)
        {
            V* dstT = static_cast<V*>(itemDst);
            *dstT = pos->curPos->second;
        }
    }

    void ItemValueSet(Iterator i, void* itemSrc) const
    {
        CollectionPos* pos = static_cast<CollectionPos*>(i);
        if (nullptr != pos)
        {
            V* srcT = static_cast<V*>(itemSrc);
            pos->curPos->second = *srcT;
        }
    }

    void* ItemPointer(Iterator i) const
    {
        void* p = nullptr;
        CollectionPos* pos = static_cast<CollectionPos*>(i);

        if (nullptr != pos)
        {
            p = &pos->curPos->second;
        }

        return p;
    }

    const char* ItemName(Iterator i) const
    {
        return nullptr;
    }

    void* ItemData(Iterator i) const
    {
        if (ItemType()->IsPointer())
        {
            return *(static_cast<void**>(ItemPointer(i)));
        }
        else
        {
            return ItemPointer(i);
        }
    }

    MetaInfo* ItemKeyType() const
    {
        return DAVA::MetaInfo::Instance<K>();
    }

    const void* ItemKeyPointer(Iterator i) const
    {
        const void* p = nullptr;
        CollectionPos* pos = static_cast<CollectionPos*>(i);

        if (nullptr != pos)
        {
            p = &(pos->curPos->first);
        }

        return p;
    }

    const void* ItemKeyData(Iterator i) const
    {
        if (ItemKeyType()->IsPointer())
        {
            return *(static_cast<const void**>(ItemKeyPointer(i)));
        }
        else
        {
            return ItemKeyPointer(i);
        }
    }

    const InspColl* Collection() const
    {
        return this;
    }

protected:
    struct CollectionPos
    {
        typename CollectionT::iterator curPos;
        typename CollectionT::iterator endPos;
    };
};

// Функция создает IntrospectionCollection, типы выводятся автоматически
template <template <typename, typename> class Container, class T, class A>
static InspColl* CreateInspColl(Container<T, A>* t, const char* _name, const InspDesc& _desc, const size_t _offset, const MetaInfo* _type, int _flags)
{
    return new InspCollImpl<Container, T, A>(_name, _desc, _offset, _type, _flags);
}

template <template <typename, typename, typename> class Container, class K, class V, class A>
static InspColl* CreateInspColl(Container<K, V, A>* t, const char* _name, const InspDesc& _desc, const size_t _offset, const MetaInfo* _type, int _flags)
{
    return new InspKeyedCollImpl<Container, K, V, A>(_name, _desc, _offset, _type, _flags);
}
};

#endif

#include "Base/Platform.h"
#include "Base/Result.h"
#include "UnitTests/UnitTests.h"
#include "Logger/Logger.h"
#include "Reflection/ReflectionRegistrator.h"

#include <functional>

struct RelfCollectionsHolder
{
    /// VectorTest
    DAVA::Vector<int> intVector;
    DAVA::Vector<DAVA::String> stringVector;
    DAVA::Vector<int>* intPtrVector;

    /// ListTest
    DAVA::List<int> intList;
    DAVA::Map<int, int> mapColl;
    DAVA::UnorderedMap<int, int> unorderMap;

    DAVA::Set<int> intSet;
    DAVA::UnorderedSet<int> intUnorderSet;

    DAVA_REFLECTION(RelfCollectionsHolder);
};

DAVA_REFLECTION_IMPL(RelfCollectionsHolder)
{
    DAVA::ReflectionRegistrator<RelfCollectionsHolder>::Begin()
    .Field("intVector", &RelfCollectionsHolder::intVector)
    .Field("stringVector", &RelfCollectionsHolder::stringVector)
    .Field("intPtrVector", &RelfCollectionsHolder::intPtrVector)
    .Field("intList", &RelfCollectionsHolder::intList)
    .Field("mapColl", &RelfCollectionsHolder::mapColl)
    .Field("unorderMap", &RelfCollectionsHolder::unorderMap)
    .Field("intSet", &RelfCollectionsHolder::intSet)
    .Field("intUnorderSet", &RelfCollectionsHolder::intUnorderSet)
    .End();
}

DAVA_TESTCLASS (ReflectionCollectionTest)
{
    template <typename T, typename TIter>
    void CollectionTestHelper(const DAVA::Reflection& collectionRef, const TIter& startExpected, const TIter& endExpected)
    {
        DAVA::Vector<DAVA::Reflection::Field> fields = collectionRef.GetFields();
        DAVA::Vector<T> actualData;

        std::for_each(fields.begin(), fields.end(),
                      [&actualData](const DAVA::Reflection::Field& field)
                      {
                          actualData.push_back(field.ref.GetValue().Get<T>());
                      });

        TEST_VERIFY(actualData.size() == std::distance(startExpected, endExpected));
        TEST_VERIFY(std::equal(startExpected, endExpected, actualData.begin()));
    }

    template <typename K, typename T, typename TIter>
    void CollectionMapTestHelper(const DAVA::Reflection& collectionRef, const TIter& startExpected, const TIter& endExpected)
    {
        DAVA::Vector<DAVA::Reflection::Field> fields = collectionRef.GetFields();
        DAVA::Map<K, T> actualData;

        std::for_each(fields.begin(), fields.end(),
                      [&actualData](const DAVA::Reflection::Field& field)
                      {
                          actualData.emplace(field.key.Cast<K>(), field.ref.GetValue().Get<T>());
                      });

        TEST_VERIFY(actualData.size() == std::distance(startExpected, endExpected));
        for (TIter i = startExpected; i != endExpected; ++i)
        {
            TEST_VERIFY(actualData.count(i->first) > 0);
            TEST_VERIFY(actualData[i->first] == i->second);
        }
    }

    template <typename T, typename TIter>
    void CollectionSetTestHelper(const DAVA::Reflection& collectionRef, const TIter& startExpected, const TIter& endExpected)
    {
        DAVA::Vector<DAVA::Reflection::Field> fields = collectionRef.GetFields();
        DAVA::Set<T> actualData;

        std::for_each(fields.begin(), fields.end(),
                      [&actualData](const DAVA::Reflection::Field& field)
                      {
                          actualData.emplace(field.ref.GetValue().Get<T>());
                      });

        TEST_VERIFY(actualData.size() == std::distance(startExpected, endExpected));
        for (TIter i = startExpected; i != endExpected; ++i)
        {
            TEST_VERIFY(actualData.count(*i) > 0);
        }
    }

    template <typename TIter>
    void AddInsertRemoveTest(const DAVA::Reflection& ref, const TIter& startExpected, const TIter& endExpected)
    {
        std::ptrdiff_t size = std::distance(startExpected, endExpected);
        TEST_VERIFY(size >= 0);

        for (size_t i = 0; i < static_cast<size_t>(size); ++i)
        {
            TEST_VERIFY(*(std::next(startExpected, i)) == ref.GetField(i).GetValue().Cast<int>());
        }

        const DAVA::Reflection::FieldCaps& caps = ref.GetFieldsCaps();
        TEST_VERIFY(caps.canAddField);
        TEST_VERIFY(caps.canInsertField);
        TEST_VERIFY(caps.canRemoveField);

        TEST_VERIFY(ref.AddField(DAVA::Any(), int(5)));
        TEST_VERIFY(ref.GetField(size_t(5)).GetValue().Cast<int>() == 5);
        TEST_VERIFY(ref.AddField(DAVA::Any(), int(6)));
        TEST_VERIFY(ref.GetField(size_t(6)).GetValue().Cast<int>() == 6);
        TEST_VERIFY(ref.InsertField(size_t(6), DAVA::Any(), int(7)));
        TEST_VERIFY(ref.GetField(size_t(5)).GetValue().Cast<int>() == 5);
        TEST_VERIFY(ref.GetField(size_t(6)).GetValue().Cast<int>() == 7);
        TEST_VERIFY(ref.GetField(size_t(7)).GetValue().Cast<int>() == 6);

        TEST_VERIFY(ref.RemoveField(size_t(7)));
        TEST_VERIFY(ref.RemoveField(size_t(5)));
        TEST_VERIFY(ref.RemoveField(size_t(5)));
    }

    template <typename TIter>
    void AddInsertRemoveMapTest(const DAVA::Reflection& ref, const TIter& startExpected, const TIter& endExpected)
    {
        for (TIter i = startExpected; i != endExpected; ++i)
        {
            DAVA::Reflection r = ref.GetField(i->first);
            int v = r.GetValue().Cast<int>();
            TEST_VERIFY(i->second == v);
        }

        const DAVA::Reflection::FieldCaps& caps = ref.GetFieldsCaps();
        TEST_VERIFY(caps.canAddField);
        TEST_VERIFY(!caps.canInsertField);
        TEST_VERIFY(caps.canRemoveField);

        TEST_VERIFY(ref.AddField(int(5), int(5)));
        TEST_VERIFY(ref.GetField(int(5)).GetValue().Cast<int>() == 5);
        TEST_VERIFY(ref.AddField(int(6), int(6)));
        TEST_VERIFY(ref.GetField(int(6)).GetValue().Cast<int>() == 6);
        TEST_VERIFY(ref.AddField(int(7), int(7)));
        TEST_VERIFY(ref.GetField(int(5)).GetValue().Cast<int>() == 5);
        TEST_VERIFY(ref.GetField(int(6)).GetValue().Cast<int>() == 6);
        TEST_VERIFY(ref.GetField(int(7)).GetValue().Cast<int>() == 7);

        TEST_VERIFY(ref.RemoveField(int(7)));
        TEST_VERIFY(ref.RemoveField(int(5)));
        TEST_VERIFY(!ref.RemoveField(int(5)));
    }

    template <typename TIter>
    void AddInsertRemoveSetTest(const DAVA::Reflection& ref, const TIter& startExpected, const TIter& endExpected)
    {
        for (TIter i = startExpected; i != endExpected; ++i)
        {
            DAVA::Reflection r = ref.GetField(*i);
            int v = r.GetValue().Cast<int>();
            TEST_VERIFY((*i) == v);
        }

        const DAVA::Reflection::FieldCaps& caps = ref.GetFieldsCaps();
        TEST_VERIFY(caps.canAddField);
        TEST_VERIFY(!caps.canInsertField);
        TEST_VERIFY(caps.canRemoveField);

        TEST_VERIFY(ref.AddField(DAVA::Any(), int(5)));
        TEST_VERIFY(ref.GetField(int(5)).GetValue().Cast<int>() == 5);
        TEST_VERIFY(ref.AddField(DAVA::Any(), int(6)));
        TEST_VERIFY(ref.GetField(int(6)).GetValue().Cast<int>() == 6);
        TEST_VERIFY(ref.AddField(DAVA::Any(), int(7)));
        TEST_VERIFY(ref.GetField(int(5)).GetValue().Cast<int>() == 5);
        TEST_VERIFY(ref.GetField(int(6)).GetValue().Cast<int>() == 6);
        TEST_VERIFY(ref.GetField(int(7)).GetValue().Cast<int>() == 7);

        TEST_VERIFY(ref.RemoveField(int(7)));
        TEST_VERIFY(ref.RemoveField(int(5)));
        TEST_VERIFY(!ref.RemoveField(int(5)));

        DAVA::Reflection r = ref.GetField(int(0));
        TEST_VERIFY(r.IsReadonly());
        TEST_VERIFY(!r.SetValue(int(10)));
    }

    DAVA_TEST (VectorTest)
    {
        int testIntData[] = { 0, 1, 2, 3, 4 };
        DAVA::String testStringData[] = { "0", "0", "0", "0" };
        RelfCollectionsHolder holder;
        std::for_each(std::begin(testIntData), std::end(testIntData), [&holder](int v) { holder.intVector.push_back(v); });
        std::for_each(std::begin(testStringData), std::end(testStringData), [&holder](const DAVA::String& v) { holder.stringVector.push_back(v); });
        holder.intPtrVector = &holder.intVector;

        DAVA::Reflection r = DAVA::Reflection::Create(&holder);
        CollectionTestHelper<int>(r.GetField("intPtrVector"), holder.intVector.begin(), holder.intVector.end());
        CollectionTestHelper<DAVA::String>(r.GetField("stringVector"), holder.stringVector.begin(), holder.stringVector.end());

        DAVA::Reflection rf = r.GetField("intVector");
        AddInsertRemoveTest(rf, holder.intVector.begin(), holder.intVector.end());
        CollectionTestHelper<int>(rf, holder.intVector.begin(), holder.intVector.end());
    }

    DAVA_TEST (ListTest)
    {
        int testIntData[] = { 0, 1, 2, 3, 4 };
        RelfCollectionsHolder holder;
        std::for_each(std::begin(testIntData), std::end(testIntData), [&holder](int v) { holder.intList.push_back(v); });

        DAVA::Reflection r = DAVA::Reflection::Create(&holder);
        DAVA::Reflection listField = r.GetField("intList");
        CollectionTestHelper<int>(listField, holder.intList.begin(), holder.intList.end());
        AddInsertRemoveTest(listField, holder.intList.begin(), holder.intList.end());
    }

    DAVA_TEST (MapTest)
    {
        int testIntData[] = { 0, 1, 2, 3, 4 };
        RelfCollectionsHolder holder;
        std::for_each(std::begin(testIntData), std::end(testIntData), [&holder](int v) { holder.mapColl.emplace(v, v + 10); });

        DAVA::Reflection r = DAVA::Reflection::Create(&holder);
        DAVA::Reflection mapField = r.GetField("mapColl");
        CollectionMapTestHelper<int, int>(mapField, holder.mapColl.begin(), holder.mapColl.end());
        AddInsertRemoveMapTest(mapField, holder.mapColl.begin(), holder.mapColl.end());
        CollectionMapTestHelper<int, int>(mapField, holder.mapColl.begin(), holder.mapColl.end());
    }

    DAVA_TEST (UnorderedMapTest)
    {
        int testIntData[] = { 0, 1, 2, 3, 4 };
        RelfCollectionsHolder holder;
        std::for_each(std::begin(testIntData), std::end(testIntData), [&holder](int v) { holder.unorderMap.emplace(v, v + 10); });

        DAVA::Reflection r = DAVA::Reflection::Create(&holder);
        DAVA::Reflection mapField = r.GetField("unorderMap");
        CollectionMapTestHelper<int, int>(mapField, holder.unorderMap.begin(), holder.unorderMap.end());
        AddInsertRemoveMapTest(mapField, holder.unorderMap.begin(), holder.unorderMap.end());
        CollectionMapTestHelper<int, int>(mapField, holder.unorderMap.begin(), holder.unorderMap.end());
    }

    DAVA_TEST (SetTest)
    {
        int testIntData[] = { 0, 1, 2, 3, 4 };
        RelfCollectionsHolder holder;
        std::for_each(std::begin(testIntData), std::end(testIntData), [&holder](int v) { holder.intSet.emplace(v); });

        DAVA::Reflection r = DAVA::Reflection::Create(&holder);
        DAVA::Reflection setField = r.GetField("intSet");
        CollectionSetTestHelper<int>(setField, holder.intSet.begin(), holder.intSet.end());
        AddInsertRemoveSetTest(setField, holder.intSet.begin(), holder.intSet.end());
        CollectionSetTestHelper<int>(setField, holder.intSet.begin(), holder.intSet.end());
    }

    DAVA_TEST (UnorderSetTest)
    {
        int testIntData[] = { 0, 1, 2, 3, 4 };
        RelfCollectionsHolder holder;
        std::for_each(std::begin(testIntData), std::end(testIntData), [&holder](int v) { holder.intUnorderSet.emplace(v); });

        DAVA::Reflection r = DAVA::Reflection::Create(&holder);
        DAVA::Reflection setField = r.GetField("intUnorderSet");
        CollectionSetTestHelper<int>(setField, holder.intUnorderSet.begin(), holder.intUnorderSet.end());
        AddInsertRemoveSetTest(setField, holder.intUnorderSet.begin(), holder.intUnorderSet.end());
        CollectionSetTestHelper<int>(setField, holder.intUnorderSet.begin(), holder.intUnorderSet.end());
    }
};

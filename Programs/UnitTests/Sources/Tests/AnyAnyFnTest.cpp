#include "Base/Any.h"
#include "Base/AnyFn.h"
#include "Base/TypeInheritance.h"
#include "Math/Vector.h"
#include "UnitTests/UnitTests.h"
#include <numeric>

namespace DAVA
{
DAVA_TESTCLASS (AnyAnyFnTest)
{
    struct Stub
    {
    };

    struct Trivial
    {
        bool operator==(const Trivial& t) const
        {
            return (a == t.a && b == t.b && c == t.c);
        }

        int a;
        int b;
        int c;
    };

    struct NotTrivial
    {
        virtual ~NotTrivial()
        {
        }
    };

    struct A
    {
        enum E1
        {
            E1_1,
            E1_2
        };

        enum class E2
        {
            E2_1,
            E2_2
        };

        int a = 1;

        Vector3 TestFn(Vector3 v1, Vector3 v2)
        {
            return (v1 * v2) * static_cast<float32>(a);
        }

        Vector3 TestFnConst(Vector3 v1, Vector3 v2) const
        {
            return (v1 * v2);
        }

        static int32 StaticTestFn(int32 a, int32 b)
        {
            return a + b;
        }

        template <typename R, typename... T>
        R TestSum(const T&... args)
        {
            auto fn = [](const std::array<R, sizeof...(args)>& a) -> R
            {
                return std::accumulate(a.begin(), a.end(), R(0));
            };

            return fn({ args... });
        }
    };

    class A1
    {
    public:
        int a1;
    };

    class B : public A
    {
    public:
        int b;
    };

    class D : public A, public A1
    {
    public:
        int d;
    };

    class E : public D
    {
    public:
        int e;
    };

    template <typename T>
    void DoAutoStorageSimpleTest(const T& initialValue)
    {
        T simpleValue(initialValue);

        AutoStorage<> as;
        TEST_VERIFY(as.IsEmpty());

        as.SetSimple(simpleValue);
        TEST_VERIFY(!as.IsEmpty());
        TEST_VERIFY(as.IsSimple());
        TEST_VERIFY(as.GetSimple<T>() == simpleValue);
        TEST_VERIFY(as.GetSimple<const T&>() == simpleValue);

        const T& simpleRef = simpleValue;
        as.SetSimple(simpleRef);
        TEST_VERIFY(!as.IsEmpty());
        TEST_VERIFY(as.IsSimple());
        TEST_VERIFY(as.GetSimple<T>() == simpleRef);
        TEST_VERIFY(as.GetSimple<const T&>() == simpleRef);

        as.Clear();
        TEST_VERIFY(as.IsEmpty());
    }

    template <typename T>
    void DoAutoStorageSharedTest(const T& initialValue)
    {
        T sharedValue(initialValue);

        AutoStorage<> as;
        TEST_VERIFY(as.IsEmpty());

        as.SetShared(sharedValue);
        TEST_VERIFY(!as.IsEmpty());
        TEST_VERIFY(!as.IsSimple());
        TEST_VERIFY(as.GetShared<T>() == sharedValue);
        TEST_VERIFY(as.GetShared<const T&>() == sharedValue);

        const T& sharedRef = sharedValue;
        as.SetShared(sharedRef);
        TEST_VERIFY(!as.IsEmpty());
        TEST_VERIFY(!as.IsSimple());
        TEST_VERIFY(as.GetShared<T>() == sharedRef);
        TEST_VERIFY(as.GetShared<const T&>() == sharedRef);

        as.Clear();
        TEST_VERIFY(as.IsEmpty());
    }

    template <typename T>
    void DoAnyTest(const T& initialValue)
    {
        T value(initialValue);
        T& valueRef(value);
        const T& valueConstRef(value);

        Any a;
        TEST_VERIFY(a.IsEmpty());

        a.Set(value);
        TEST_VERIFY(!a.IsEmpty());
        TEST_VERIFY(value == a.Get<T>());
        TEST_VERIFY(value == a.Get<const T&>());

        a.Set(valueRef);
        TEST_VERIFY(!a.IsEmpty());
        TEST_VERIFY(value == a.Get<T>());
        TEST_VERIFY(value == a.Get<const T&>());

        a.Set(valueConstRef);
        TEST_VERIFY(!a.IsEmpty());
        TEST_VERIFY(value == a.Get<T>());
        TEST_VERIFY(value == a.Get<const T&>());

        a.Clear();
        TEST_VERIFY(a.IsEmpty());
        TEST_VERIFY(!a.CanGet<int>());
        TEST_VERIFY(!a.CanGet<void*>());

        TEST_VERIFY(123 == a.Get<int>(123));

        try
        {
            a.Get<int>();
            TEST_VERIFY(false && "Shouldn't be able to get int from empty Any");
        }
        catch (const Exception& e)
        {
            TEST_VERIFY(e.callstack.size() > 0);
        }
    }

    template <typename T1, typename T2>
    void DoAnyCastTest()
    {
        int v = 123;
        T1 t1 = static_cast<T1>(v);

        Any a(t1);
        T2 t2 = a.Cast<T2>();

        Any b(t2);
        T1 t3 = a.Cast<T1>();

        TEST_VERIFY(t3 == t1);
        TEST_VERIFY(!a.CanCast<Stub>());
        TEST_VERIFY(!b.CanCast<Stub>());
    }

    DAVA_TEST (AutoStorageTest)
    {
        int32 v = 10203040;

        DoAutoStorageSimpleTest<int32>(v);
        DoAutoStorageSimpleTest<int64>(10203040506070);
        DoAutoStorageSimpleTest<float32>(0.123456f);
        DoAutoStorageSimpleTest<void*>(nullptr);
        DoAutoStorageSimpleTest<int32*>(&v);
        DoAutoStorageSimpleTest<const int32*>(&v);

        String s("10203040");

        DoAutoStorageSharedTest<int32>(v);
        DoAutoStorageSharedTest<int64>(10203040506070);
        DoAutoStorageSharedTest<float32>(0.123456f);
        DoAutoStorageSharedTest<String>(s);
        DoAutoStorageSharedTest<void*>(nullptr);
        DoAutoStorageSharedTest<String*>(&s);
        DoAutoStorageSharedTest<const String*>(&s);
    }

    DAVA_TEST (TypeTest)
    {
        struct ccc
        {
        };

        ccc c1;
        ccc c2;

        const Type* t1 = Type::Instance<ccc&>();
        const Type* t2 = Type::Instance<ccc const&>();
        const Type* t3 = Type::Instance<const ccc&>();

        const Type* tp1 = Type::Instance<ccc*>();
        const Type* tp2 = Type::Instance<ccc const*>();
        const Type* tp3 = Type::Instance<const ccc* const>();

        TEST_VERIFY(t1 != t2);
        TEST_VERIFY(t2 == t3);
        TEST_VERIFY(t1->GetTypeFlags() != t2->GetTypeFlags());

        TEST_VERIFY(tp1 != tp2);
        TEST_VERIFY(tp1 != tp3);
        TEST_VERIFY(tp2 != tp3);
        TEST_VERIFY(tp1->GetTypeFlags() != tp2->GetTypeFlags());
        TEST_VERIFY(tp1->GetTypeFlags() != tp3->GetTypeFlags());
        TEST_VERIFY(tp2->GetTypeFlags() != tp3->GetTypeFlags());

        uint32_t customIndex = Type::AllocUserData();

        TEST_VERIFY(nullptr == Type::Instance<int>()->GetUserData(customIndex));
        TEST_VERIFY(nullptr == Type::Instance<float>()->GetUserData(customIndex));

        Type::Instance<int>()->SetUserData(customIndex, &c1);
        Type::Instance<float>()->SetUserData(customIndex, &c2);

        TEST_VERIFY(&c1 == Type::Instance<int>()->GetUserData(customIndex));
        TEST_VERIFY(&c1 != Type::Instance<float>()->GetUserData(customIndex));
        TEST_VERIFY(&c2 != Type::Instance<int>()->GetUserData(customIndex));
        TEST_VERIFY(&c2 == Type::Instance<float>()->GetUserData(customIndex));

        Type::Instance<int>()->SetUserData(customIndex, nullptr);
        Type::Instance<float>()->SetUserData(customIndex, nullptr);

        TEST_VERIFY(nullptr == Type::Instance<int>()->GetUserData(customIndex));
        TEST_VERIFY(nullptr == Type::Instance<float>()->GetUserData(customIndex));
    }

    DAVA_TEST (AnyMove)
    {
        int32 v = 516273;
        String s("516273");

        Any a(v);
        Any b(std::move(a));
        TEST_VERIFY(a.IsEmpty());
        TEST_VERIFY(!b.IsEmpty());
        TEST_VERIFY(b.Get<decltype(v)>() == v);

        a.Set(s);
        b.Set(std::move(a));
        TEST_VERIFY(a.IsEmpty());
        TEST_VERIFY(!b.IsEmpty());
        TEST_VERIFY(b.Get<decltype(s)>() == s);
    }

    DAVA_TEST (AnyTestSimple)
    {
        int32 v = 50607080;
        String s("50607080");

        DoAnyTest<int32>(v);
        DoAnyTest<int64>(80706050403020);
        DoAnyTest<float32>(0.123456f);
        DoAnyTest<void*>(nullptr);
        DoAnyTest<const void*>(nullptr);
        DoAnyTest<const int32&>(v);
        DoAnyTest<int32*>(&v);
        DoAnyTest<const int32*>(&v);
        DoAnyTest<String>(s);
        DoAnyTest<const String&>(s);
        DoAnyTest<String*>(&s);
        DoAnyTest<const String*>(&s);

        try
        {
            Any a(10);
            a.Get<String>();
            TEST_VERIFY(false && "Shouldn't be able to ge String");
        }
        catch (const Exception&)
        {
            TEST_VERIFY(true);
        }
    }

    DAVA_TEST (EmptyAnyCastGetTest)
    {
        Any a;
        TEST_VERIFY(a.CanGet<int32>() == false);
        TEST_VERIFY(a.CanCast<int32>() == false);

        TEST_VERIFY(a != Any(3));
        TEST_VERIFY(Any(3) != a);
        TEST_VERIFY(Any() == Any()); //-V501
        TEST_VERIFY((Any() != Any()) == false); //-V501
    }

    DAVA_TEST (AnyTestPtr)
    {
        B b;
        D d;
        E e;

        B* bPtr = &b;
        A* baPtr = static_cast<A*>(bPtr);

        D* dPtr = &d;
        A* daPtr = static_cast<A*>(dPtr);
        A1* da1Ptr = static_cast<A1*>(dPtr);

        E* ePtr = &e;
        A1* ea1Ptr = static_cast<A1*>(ePtr);

        Any a;
        a.Set(bPtr);

        TypeInheritance::RegisterBases<B, A>();
        TypeInheritance::RegisterBases<D, A, A1>();
        TypeInheritance::RegisterBases<E, D>();

        // simple
        TEST_VERIFY(bPtr == a.Get<void*>());
        TEST_VERIFY(bPtr == a.Get<const void*>());
        TEST_VERIFY(bPtr == a.Get<B*>());
        TEST_VERIFY(bPtr == a.Get<const B*>());
        TEST_VERIFY(a.CanCast<A*>());
        TEST_VERIFY(a.CanCast<const A*>());
        TEST_VERIFY(baPtr == a.Cast<A*>());
        TEST_VERIFY(baPtr == a.Cast<const A*>());

        TEST_VERIFY(!a.CanCast<Stub*>());
        TEST_VERIFY(!a.CanCast<const Stub*>());
        TEST_VERIFY(!a.CanCast<int>());
        TEST_VERIFY(!a.CanCast<void>());
        TEST_VERIFY(!a.CanCast<Stub>());

        // multiple inheritance
        a.Set(dPtr);
        TEST_VERIFY(a.CanCast<A*>());
        TEST_VERIFY(a.CanCast<A1*>());
        TEST_VERIFY(dPtr == a.Get<void*>());
        TEST_VERIFY(daPtr == a.Cast<A*>());
        TEST_VERIFY(da1Ptr == a.Cast<A1*>());

        // hierarchy down cast
        a.Set(ePtr);
        TEST_VERIFY(a.CanCast<A1*>());
        TEST_VERIFY(ea1Ptr == a.Cast<A1*>());

        // hierarchy up cast
        a.Set(ea1Ptr);
        TEST_VERIFY(ePtr == a.Cast<E*>());

        try
        {
            a.Cast<int>();
            TEST_VERIFY(false && "Shouldn't be able to cast to int");
        }
        catch (const Exception&)
        {
            TEST_VERIFY(true);
        }
    }

    DAVA_TEST (AnyTestEnum)
    {
        Any a;

        a.Set(A::E1_1);
        TEST_VERIFY(A::E1_1 == a.Get<A::E1>());
        TEST_VERIFY(A::E1_2 != a.Get<A::E1>());

        a.Set(A::E2::E2_2);
        TEST_VERIFY(A::E2::E2_1 != a.Get<A::E2>());
        TEST_VERIFY(A::E2::E2_2 == a.Get<A::E2>());
    }

    DAVA_TEST (AnyLoadStoreCompare)
    {
        int v1 = 11223344;
        int v2 = 321;

        int* iptr1 = &v1;
        int* iptr2 = nullptr;

        // two values ==
        Any a(v1);
        Any b(v1);
        TEST_VERIFY(a == b);

        Any a1(a);
        Any b1(b);
        TEST_VERIFY(a1 == b1);

        // two values !=
        a.Set(v1);
        b.Set(String("str"));
        TEST_VERIFY(a != b);

        a1.Set(a);
        b1.Set(b);
        TEST_VERIFY(a1 != b1);

        // two pointer !=
        a.Set(&v1);
        b.Set(&v2);
        TEST_VERIFY(a != b);

        // pointer and value !=
        a.Set(v1);
        b.Set(&v2);
        TEST_VERIFY(a != b);

        // empty != value
        a.Clear();
        b.Set(v1);
        TEST_VERIFY(a != b);

        // empty != pointer
        a.Clear();
        b.Set(&v1);
        TEST_VERIFY(a != b);

        // empty != nullptr
        a.Clear();
        b.Set(nullptr);
        TEST_VERIFY(a != b);

        // two empties ==
        a.Clear();
        b.Clear();
        TEST_VERIFY(a == b);

        // load test
        a.LoadData(&v1, Type::Instance<int>());
        TEST_VERIFY(a.Get<int>() == v1);

        // store test
        a.StoreData(&v2, sizeof(v2));
        TEST_VERIFY(v1 == v2);

        // load/store pointers
        a.LoadData(&iptr1, Type::Instance<int*>());
        a.StoreData(&iptr2, sizeof(iptr2));
        TEST_VERIFY(iptr1 == iptr2);

        // load/store trivial types
        Trivial triv;
        Trivial triv1{ 11, 22 };
        a.LoadData(&triv, Type::Instance<Trivial>());
        TEST_VERIFY(a.Get<Trivial>() == triv);
        a.StoreData(&triv1, sizeof(triv1));
        TEST_VERIFY(triv1 == triv);

        // load/store fail cases
        NotTrivial not_triv;
        TEST_VERIFY(!a.LoadData(&not_triv, Type::Instance<NotTrivial>()));
        TEST_VERIFY(!a.StoreData(&not_triv, sizeof(not_triv)));
        TEST_VERIFY(!a.StoreData(&triv, sizeof(triv) / 2));
    }

    DAVA_TEST (AnyCastTest)
    {
#if 0
        DoAnyCastTest<int8, int16>();
        DoAnyCastTest<int8, int32>();
        DoAnyCastTest<int8, int64>();
        DoAnyCastTest<int8, uint8>();
        DoAnyCastTest<int8, uint16>();
        DoAnyCastTest<int8, uint32>();
        DoAnyCastTest<int8, uint64>();
        DoAnyCastTest<int8, float32>();
        DoAnyCastTest<int8, float64>();
        DoAnyCastTest<int8, size_t>();

        DoAnyCastTest<int16, int32>();
        DoAnyCastTest<int16, int64>();
        DoAnyCastTest<int16, uint8>();
        DoAnyCastTest<int16, uint16>();
        DoAnyCastTest<int16, uint32>();
        DoAnyCastTest<int16, uint64>();
        DoAnyCastTest<int16, float32>();
        DoAnyCastTest<int16, float64>();
        DoAnyCastTest<int16, size_t>();

        DoAnyCastTest<int32, int64>();
        DoAnyCastTest<int32, uint8>();
        DoAnyCastTest<int32, uint16>();
        DoAnyCastTest<int32, uint32>();
        DoAnyCastTest<int32, uint64>();
        DoAnyCastTest<int32, float32>();
        DoAnyCastTest<int32, float64>();
        DoAnyCastTest<int32, size_t>();

        DoAnyCastTest<int64, uint8>();
        DoAnyCastTest<int64, uint16>();
        DoAnyCastTest<int64, uint32>();
        DoAnyCastTest<int64, uint64>();
        DoAnyCastTest<int64, float32>();
        DoAnyCastTest<int64, float64>();
        DoAnyCastTest<int64, size_t>();

        DoAnyCastTest<uint8, uint16>();
        DoAnyCastTest<uint8, uint32>();
        DoAnyCastTest<uint8, uint64>();
        DoAnyCastTest<uint8, float32>();
        DoAnyCastTest<uint8, float64>();
        DoAnyCastTest<uint8, size_t>();

        DoAnyCastTest<uint16, uint32>();
        DoAnyCastTest<uint16, uint64>();
        DoAnyCastTest<uint16, float32>();
        DoAnyCastTest<uint16, float64>();
        DoAnyCastTest<uint16, size_t>();

        DoAnyCastTest<uint32, uint64>();
        DoAnyCastTest<uint32, float32>();
        DoAnyCastTest<uint32, float64>();
        DoAnyCastTest<uint32, size_t>();

        DoAnyCastTest<uint64, float32>();
        DoAnyCastTest<uint64, float64>();
        DoAnyCastTest<uint64, size_t>();

        DoAnyCastTest<float32, float64>();
        DoAnyCastTest<float32, size_t>();

        DoAnyCastTest<float64, size_t>();
#endif
    }

    DAVA_TEST (AnyFnTest)
    {
        A a;

        AnyFn fn;
        TEST_VERIFY(!fn.IsValid())

        fn = AnyFn(&A::StaticTestFn);
        TEST_VERIFY(fn.IsValid());
        TEST_VERIFY(fn.IsStatic());
        TEST_VERIFY(A::StaticTestFn(10, 20) == fn.Invoke(Any(10), Any(20)).Get<int32>());

        try
        {
            fn.Invoke();
            TEST_VERIFY(false && "Shouldn't be invoked with bad arguments");
        }
        catch (const Exception&)
        {
            TEST_VERIFY(true);
        }

        try
        {
            fn.BindThis(&a);
            TEST_VERIFY(false && "This shouldn't be binded to static function");
        }
        catch (const Exception&)
        {
            TEST_VERIFY(true);
        }

        fn = AnyFn(&A::TestFn);
        TEST_VERIFY(fn.IsValid());
        TEST_VERIFY(!fn.IsStatic());

        Vector3 op1(1.0f, 2.0f, 3.0f);
        Vector3 op2(7.0f, 0.7f, 0.0f);
        Vector3 op3(4.0f, 3.5f, 2.1f);

        Any res;

        fn.Invoke(Any(&a), Any(op1), Any(op2));
        res = fn.Invoke(Any(static_cast<const A*>(&a)), Any(op1), Any(op2));
        TEST_VERIFY(a.TestFn(op1, op2) == res.Get<Vector3>());

        // check compilation if pointer is const
        TEST_VERIFY(a.TestFn(op1, op2) == res.Get<Vector3>());

        fn.BindThis(&a);
        fn = fn.BindThis(static_cast<const A*>(&a));
        TEST_VERIFY(fn.IsValid());
        TEST_VERIFY(fn.IsStatic());

        a.a = 5;
        res = fn.Invoke(Any(op1), Any(op3));
        TEST_VERIFY(a.TestFn(op1, op3) == res.Get<Vector3>());

        fn = AnyFn(&A::TestFnConst);
        res = fn.Invoke(Any(&a), Any(op2), Any(op3));
        TEST_VERIFY(a.TestFnConst(op2, op3) == res.Get<Vector3>());
    }

    template <typename R, typename... T>
    void DoAnyFnInvokeTest(const T&... args)
    {
        A a;
        AnyFn fn(&A::TestSum<R, T...>);

        auto& invokeParams = fn.GetInvokeParams();
        TEST_VERIFY(invokeParams.retType == Type::Instance<R>());
        TEST_VERIFY(invokeParams.argsType.at(0) == Type::Instance<A*>());
        TEST_VERIFY(invokeParams.argsType.size() == (sizeof...(args) + 1));

        Any res = fn.Invoke(&a, args...);
        TEST_VERIFY(res.Get<R>() == a.TestSum<R>(args...));

        try
        {
            fn.Invoke(nullptr, nullptr);
            TEST_VERIFY(false && "AnyFn shouldn't invoke with bad arguments");
        }
        catch (const Exception&)
        {
            TEST_VERIFY(true);
        }

        // now bind this, and test once again
        fn = fn.BindThis(&a);
        auto& invokeParams1 = fn.GetInvokeParams();
        TEST_VERIFY(invokeParams1.retType == Type::Instance<R>());
        TEST_VERIFY(invokeParams1.argsType.size() == sizeof...(args));

        res = fn.Invoke(args...);
        TEST_VERIFY(res.Get<R>() == a.TestSum<R>(args...));
    }

    DAVA_TEST (AnyFnInvokeTest)
    {
        DoAnyFnInvokeTest<int>(1);
        DoAnyFnInvokeTest<int>(1, 2);
        DoAnyFnInvokeTest<int>(1, 2, 3);
        DoAnyFnInvokeTest<int>(1, 2, 3, 4);
        DoAnyFnInvokeTest<int>(1, 2, 3, 4, 5);

        DoAnyFnInvokeTest<float>(10.0f);
        DoAnyFnInvokeTest<float>(10.0f, 0.2f);
        DoAnyFnInvokeTest<float>(10.0f, 0.2f, 3.0f);
        DoAnyFnInvokeTest<float>(10.0f, 0.2f, 3.0f, 0.04f);
        DoAnyFnInvokeTest<float>(10.0f, 0.2f, 3.0f, 0.04f, 500.0f);
    }
};

} // namespace DAVA

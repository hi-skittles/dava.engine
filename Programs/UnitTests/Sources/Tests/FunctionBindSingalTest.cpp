#include "Logger/Logger.h"
#include "Functional/Signal.h"
#include "UnitTests/UnitTests.h"

// =======================================================================================================================================
// =======================================================================================================================================
// =======================================================================================================================================

namespace FunctionBindDetails
{
using namespace DAVA;

static String functionBindSignalResultString;

void staticFn0Void()
{
}

int staticFn0()
{
    return 100;
}

int staticFn3(int i1, int i2, int i3)
{
    return i1 + i2 + i3;
}

void* exStaticFnV(void* a)
{
    return a;
}
void* exStaticFnVV(void** a)
{
    return *a;
}
const char* exStaticFnCC(const char* a, int b)
{
    return a + b;
}
const char* exStaticFnCCC(const char** a, int b)
{
    return *a + b;
}

struct A
{
    int classFn0()
    {
        return 100;
    }
    int classFn3(int i1, int i2, int i3)
    {
        return i1 + i2 + i3;
    }

    int classFn0_const() const
    {
        return 100;
    }
    int classFn3_const(int i1, int i2, int i3) const
    {
        return i1 + i2 + i3;
    }

    int incomingFunction(Function<int()> fn)
    {
        return fn();
    }
    int incomingFunctionRef(Function<int()>& fn)
    {
        return fn();
    }
    int incomingFunctionConstRef(const Function<int()>& fn)
    {
        return fn();
    }

    void setV(int value)
    {
        v = value;
    }
    int getV()
    {
        return v;
    }

    int v;
};

struct B : public A
{
    A* exClassFn(A* a)
    {
        return a;
    }
    const A* exClassFn1(const A* a)
    {
        return a;
    }
    A* exClassFn2(A** a)
    {
        return *a;
    }
    const A* exClassFn3(const A** a)
    {
        return *a;
    }

    int getV()
    {
        return v * v;
    }
};

struct P
{
    char c_[128];
    int getV()
    {
        return 10;
    }
};

struct V
{
    virtual ~V() = default;
    virtual int f2virt(int i, long j) = 0;
    char c_[2];
};

struct C : public P, virtual public V, virtual public A
{
    virtual ~C() = default;
    virtual void f2()
    {
    }
    virtual int f2defvirt(int i, long j)
    {
        return static_cast<int>(i + j + 1);
    }
    int f2def(int i, long j)
    {
        return static_cast<int>(i + j + 3);
    }
    virtual int f2virt(int i, long j)
    {
        return static_cast<int>(i + j + 2);
    }
};

struct MindChangingClass
{
    MindChangingClass(Signal<>& sig)
        : signal(sig)
    {
        id = signal.Connect(this, &MindChangingClass::Tik);
        signal.Connect(this, &MindChangingClass::Tak);
        signal.Connect(this, &MindChangingClass::Tak);
    }

    void Tik()
    {
        count++;
        signal.Disconnect(id);
    }

    void Tak()
    {
        count++;
        signal.Disconnect(this);
    }

    uint32 count = 0;
    Signal<>& signal;
    Token id;
};
} // namespace FunctionBindDetails

// =======================================================================================================================================
// =======================================================================================================================================
// =======================================================================================================================================

DAVA_TESTCLASS (FunctionBindSignalTest)
{
    DAVA_TEST (TestFunction)
    {
        using namespace DAVA;
        using namespace FunctionBindDetails;
        // ==================================================================================
        // common functions testing
        // ==================================================================================
        Function<void()> emptyProcedure(&staticFn0Void);
        Function<int()> static_f0(&staticFn0);
        Function<int(int, int, int)> static_f3(&staticFn3);

        TEST_VERIFY(static_f0() == staticFn0());
        TEST_VERIFY(static_f3(3, 8, 5) == staticFn3(3, 8, 5));

        A a;
        Function<int(A*)> class_f0 = &A::classFn0;
        Function<int(A*, int, int, int)> class_f3 = &A::classFn3;

        Function<int(A*)> class_f0c = &A::classFn0_const;
        Function<int(A*, int, int, int)> class_f3c = &A::classFn3_const;

        TEST_VERIFY(class_f0(&a) == a.classFn0());
        TEST_VERIFY(class_f3(&a, 3, 8, 5) == a.classFn3(3, 8, 5));

        TEST_VERIFY(class_f0c(&a) == a.classFn0_const());
        TEST_VERIFY(class_f3c(&a, 3, 8, 5) == a.classFn3_const(3, 8, 5));

        // ==================================================================================
        // std::swap test
        // ==================================================================================
        auto sw_la1 = [] { return 1; };
        auto sw_la2 = [] { return 2; };
        Function<int()> fn_swap1(sw_la1);
        Function<int()> fn_swap2(sw_la2);

        std::swap(fn_swap1, fn_swap2);
        TEST_VERIFY(fn_swap1() == sw_la2());
        TEST_VERIFY(fn_swap2() == sw_la1());

        // ==================================================================================
        // function with assigned object
        // ==================================================================================
        Function<int()> object_f0(&a, &A::classFn0);
        Function<int(int, int, int)> object_f3(&a, &A::classFn3);

        Function<int()> object_f0c(&a, &A::classFn0_const);
        Function<int(int, int, int)> object_f3c(&a, &A::classFn3_const);

        TEST_VERIFY(object_f0() == a.classFn0());
        TEST_VERIFY(object_f3(3, 8, 5) == a.classFn3(3, 8, 5));

        TEST_VERIFY(object_f0c() == a.classFn0_const());
        TEST_VERIFY(object_f3c(3, 8, 5) == a.classFn3_const(3, 8, 5));

        // ==================================================================================
        // MakeFunction helper testing
        // ==================================================================================
        MakeFunction(&staticFn0)();
        MakeFunction(&staticFn3)(1, 2, 3);

        MakeFunction(&A::classFn0)(&a);
        MakeFunction(&A::classFn3)(&a, 1, 2, 3);

        MakeFunction(static_f3)(1, 2, 3);
        MakeFunction(class_f3)(&a, 1, 2, 3);

        // ==================================================================================
        // inherited class testing
        // ==================================================================================
        B b;
        Function<void(B*, int)> b0_class_setV = &B::setV;
        Function<void(A*, int)> b1_class_setV = &B::setV;

        Function<int(A*)> b3_class_getV = &A::getV;
        Function<int(B*)> b4_class_getV = &B::getV;
        Function<int(B*)> b5_class_getV = &A::getV;

        b0_class_setV(&b, 100);
        TEST_VERIFY(b.getV() == (100 * 100));
        b1_class_setV(&b, 200);
        TEST_VERIFY(b.getV() == (200 * 200));
        b1_class_setV(&a, 100);
        TEST_VERIFY(a.getV() == 100);
        b.setV(300);
        TEST_VERIFY(b3_class_getV(&b) == 300);
        a.setV(300);
        TEST_VERIFY(b3_class_getV(&a) == 300);
        b.setV(400);
        TEST_VERIFY(b4_class_getV(&b) == (400 * 400));
        b.setV(500);
        TEST_VERIFY(b5_class_getV(&b) == 500);

        Function<void(int)> b0_obj_setV(&b, &B::setV);
        Function<void(int)> b1_obj_setV(&b, &A::setV);
        Function<int()> b3_obj_getV(&b, &A::getV);
        Function<int()> b4_obj_getV(&b, &B::getV);

        Function<void(B*, int)> a0_class_setV = &A::setV;
        Function<void(A*, int)> a1_class_setV = &A::setV;
        Function<void(int)> a0_obj_setV(&a, &B::setV);
        Function<void(int)> a1_obj_setV(&a, &A::setV);
        Function<int()> a3_obj_getV(&a, &A::getV);

#if 0
        // thous functions should assert during compilation
        Function<int(B*)> no_b0_class_getV1 = &M::getV;
        Function<int()> no_a0_obj_getV(&a, &B::getV);
#endif

        // ==================================================================================
        // virtual functions testing
        // ==================================================================================
        C c;
        Function<void(C*)> c_f2 = &C::f2;
        Function<int(C*, int i, long j)> c_f2defvirt = &C::f2defvirt;
        Function<int(C*, int i, long j)> c_f2def = &C::f2def;
        Function<int(C*, int i, long j)> c_f2virt = &C::f2virt;

        Logger::Info("%u\n", sizeof(&C::f2defvirt));
        Logger::Info("%u\n", sizeof(&C::f2def));
        Logger::Info("%u\n", sizeof(&C::f2virt));

        c_f2(&c);
        TEST_VERIFY(c_f2defvirt(&c, 2000, 4454656) == c.f2defvirt(2000, 4454656));
        TEST_VERIFY(c_f2def(&c, 2000, 4454656) == c.f2def(2000, 4454656));
        TEST_VERIFY(c_f2virt(&c, 2000, 4454656) == c.f2virt(2000, 4454656));

        // ==================================================================================
        // type casting testing
        // ==================================================================================
        TEST_VERIFY(a.incomingFunction(static_f0) == staticFn0());
        TEST_VERIFY(a.incomingFunctionRef(static_f0) == staticFn0());
        TEST_VERIFY(a.incomingFunctionConstRef(static_f0) == staticFn0());
        TEST_VERIFY(a.incomingFunction(&staticFn0) == staticFn0());
        TEST_VERIFY(a.incomingFunctionConstRef(&staticFn0) == staticFn0());

        // ==================================================================================
        // operators
        // ==================================================================================
        Function<int()> null_f0;
        Function<int()> null_f0_1 = nullptr;

        TEST_VERIFY(null_f0 == nullptr);
        TEST_VERIFY(null_f0_1 == nullptr);

        null_f0 = static_f0;
        TEST_VERIFY(null_f0() == staticFn0());
        null_f0 = 0;
        TEST_VERIFY(null_f0 == nullptr);
        TEST_VERIFY(nullptr == null_f0);

        null_f0 = object_f0;
        TEST_VERIFY(null_f0() == object_f0());

        // ==================================================================================
        // bind testing
        // ==================================================================================
        A aa;
        Function<int()> bound_create_f0 = std::bind(&A::classFn0, &aa);
        TEST_VERIFY(bound_create_f0() == a.classFn0());

        Function<int()> bound_f0 = std::bind(class_f0, &aa);
        Function<int(A*)> bound_f0_1 = std::bind(class_f0, std::placeholders::_1);

        bound_f0();

        TEST_VERIFY(bound_f0() == class_f0(&aa));
        TEST_VERIFY(bound_f0_1(&aa) == class_f0(&aa));

        Function<int()> bound_f3 = std::bind(&A::classFn3, &aa, 10, 20, 30);
        Function<int(int)> bound_f3_1 = std::bind(&A::classFn3, &aa, std::placeholders::_1, 20, 30);
        Function<int(A*)> bound_f3_2 = std::bind(&A::classFn3, std::placeholders::_1, 10, 20, 30);
        Function<int(int, int, int, A*)> bound_f3_3 = std::bind(&A::classFn3, std::placeholders::_4, std::placeholders::_3, std::placeholders::_2, std::placeholders::_1);

        TEST_VERIFY(bound_f3() == aa.classFn3(10, 20, 30));
        TEST_VERIFY(bound_f3_1(10) == aa.classFn3(10, 20, 30));
        TEST_VERIFY(bound_f3_2(&aa) == aa.classFn3(10, 20, 30));
        TEST_VERIFY(bound_f3_3(30, 20, 10, &aa) == aa.classFn3(10, 20, 30));
    }

    DAVA_TEST (TestFunctionExtended)
    {
        using namespace DAVA;
        using namespace FunctionBindDetails;

        B b;
        A* a_pt = nullptr;
        void* void_test = nullptr;
        const char* char_test = nullptr;

        Function<void*(void*)> sta0(&exStaticFnV);
        Function<void*(void**)> sta1(&exStaticFnVV);
        Function<const char*(const char*, int)> sta2(&exStaticFnCC);
        Function<const char*(const char**, int)> sta3(&exStaticFnCCC);

        TEST_VERIFY(sta0(void_test) == exStaticFnV(void_test));
        TEST_VERIFY(sta1(&void_test) == exStaticFnVV(&void_test));
        TEST_VERIFY(sta2(char_test, 5) == exStaticFnCC(char_test, 5));
        TEST_VERIFY(sta3(&char_test, 10) == exStaticFnCCC(&char_test, 10));

        Function<A*(B*, A*)> cla0(&B::exClassFn);
        Function<const A*(B*, const A*)> cla1(&B::exClassFn1);
        Function<A*(B*, A**)> cla2(&B::exClassFn2);
        Function<const A*(B*, const A**)> cla3(&B::exClassFn3);

        TEST_VERIFY(cla0(&b, a_pt) == b.exClassFn(a_pt));
        TEST_VERIFY(cla1(&b, a_pt) == b.exClassFn1(a_pt));
        TEST_VERIFY(cla2(&b, &a_pt) == b.exClassFn2(&a_pt));
        TEST_VERIFY(cla3(&b, const_cast<const A**>(&a_pt)) == b.exClassFn3(const_cast<const A**>(&a_pt)));
    }

    class TestObjA : public DAVA::TrackedObject
    {
    public:
        int v1 = 0;
        int v2 = 0;
        void Slot1(int v)
        {
            v1 = v;
        }
        void Slot2(int v)
        {
            v2 = v;
        }
    };

    class TestObjB
    {
    public:
        int v1 = 0;
        void Slot1(int v)
        {
            v1 = v;
        }

        virtual ~TestObjB(){};
    };

    class TestObjC : public TestObjB, public DAVA::TrackedObject
    {
    };

    DAVA_TEST (TestSignals)
    {
        using namespace DAVA;
        using namespace FunctionBindDetails;
        // ==================================================================================
        // signals
        // ==================================================================================
        Signal<int> testSignal;

        {
            TestObjA* obj = new TestObjA();

            Token t1;
            Token t2;
            Token tinvalid;

            TEST_VERIFY(t1.IsEmpty());
            TEST_VERIFY(!t1);
            TEST_VERIFY(t1 == t2);

            t1 = testSignal.Connect(obj, &TestObjA::Slot1);
            t2 = testSignal.Connect(obj, &TestObjA::Slot1);

            TEST_VERIFY(!t1.IsEmpty());
            TEST_VERIFY(t1);

            TEST_VERIFY(!t2.IsEmpty());
            TEST_VERIFY(t2);

            TEST_VERIFY(t1 != t2);

            t1.Clear();
            TEST_VERIFY(t1.IsEmpty());
            TEST_VERIFY(t1 == tinvalid);

            testSignal.Disconnect(obj);
            delete obj;
        }

        {
            TestObjA* objA = new TestObjA();
            int check_v = 0;

            // will be automatically tracked,
            // objA is derived from TrackedObject
            testSignal.Connect(objA, &TestObjA::Slot1);

            testSignal.Emit(10);
            TEST_VERIFY(objA->v1 == 10);

            Token connA2 = testSignal.Connect([objA, &check_v](int v) {
                objA->Slot2(v);
                check_v = v;
            });

            // connA2 wont be automatically tracked
            // we should add it manually
            testSignal.Track(connA2, objA);

            int emitValue_1 = 20;
            testSignal.Emit(emitValue_1);
            TEST_VERIFY(objA->v1 == emitValue_1);
            TEST_VERIFY(objA->v2 == emitValue_1);

            // deleting object that is still connected to the signal
            // if that object is derived by TrackedObject it will be
            // automatically disconnected
            delete objA;
            int emitValue_2 = 222111;
            testSignal.Emit(emitValue_2); // <-- this shouldn't crash
            TEST_VERIFY(check_v != emitValue_2);
        }

        {
            TestObjB objB;
            Token connB1 = testSignal.Connect(&objB, &TestObjB::Slot1);
            testSignal.Emit(10);

            TEST_VERIFY(objB.v1 == 10);

            // TestObjB isn't derived from TrackedObject, so we
            // should disconnect it manually
            testSignal.Disconnect(connB1); // <-- if we don't do this there can be crash,
            // when user invokes Emmit after objB becomes out of scope
        }

        testSignal.Emit(10); // <-- this should crash, because we already disconnect from dead objB

        {
            TestObjC* objC = new TestObjC();
            Signal<int>* testSignal1 = new Signal<int>();

            // track signal deletion, while tracking object exists
            testSignal1->Connect(objC, &TestObjB::Slot1);
            delete testSignal1;
            delete objC; // <-- this shouldn't crash, because tracked signal will be removed when that signal was destroyed
        }

        {
            testSignal.DisconnectAll();

            TestObjC objC;
            Token connC1 = testSignal.Connect(&objC, [&testSignal, &connC1, &objC](int v) {
                objC.Slot1(v);
                testSignal.Block(connC1, true);
                testSignal.Emit(20);
            });
            testSignal.Emit(10); // <-- this shouldn't hang

            TEST_VERIFY(objC.v1 == 10);
            TEST_VERIFY(testSignal.IsBlocked(connC1) == true);

            testSignal.Disconnect(connC1);

            objC.v1 = 20;
            testSignal.Emit(10);
            TEST_VERIFY(objC.v1 != 10);
        }

        // check if signal will forward complex type gived by value into
        // multiple slots. it should make a copy of the given value for
        // each invoked slot
        {
            int v = 123321;

            auto shrd = std::make_shared<int>(v);
            std::weak_ptr<int> w = shrd;

            auto test_weakptr = [](int& v, std::weak_ptr<int> ptr) {
                if (!ptr.expired())
                {
                    std::shared_ptr<int> shared = ptr.lock();
                    v = *shared.get();
                }
                else
                {
                    v = 0;
                }
            };

            Signal<int&, std::weak_ptr<int>> sig;

            // connect twice to the same slot
            sig.Connect(test_weakptr);
            sig.Connect(test_weakptr);

            int res = 0;

            // this will call test_fn twice.
            // each call should receive separate copy of weak_ptr
            // so "res" should be equal to the "v" value
            sig.Emit(res, w);
            TEST_VERIFY(res == v);
        }

        // check disconnection correctness during signal emission
        {
            Signal<> signal;
            int localCount = 0;

            signal.Connect(this, [&localCount]() { localCount++; });
            signal.Connect([&localCount]() { localCount++; });

            MindChangingClass mco(signal);

            signal.Connect([&localCount]() { localCount++; });
            signal.Connect(this, [&localCount]() { localCount++; });

            TEST_VERIFY(mco.count == 0);

            // increment count in signal handler
            signal.Emit();
            TEST_VERIFY(mco.count == 2);
            TEST_VERIFY(localCount == 4);

            signal.Disconnect(this);
            localCount = 0;

            // second emission, mco is not connected to the signal, count isn't changed
            signal.Emit();
            TEST_VERIFY(mco.count == 2);
            TEST_VERIFY(localCount == 2);
        }
    }
};

#pragma once

#include "UnitTests/TestClassFactory.h"
#include "UnitTests/TestClass.h"
#include "UnitTests/TestCore.h"

/******************************************************************************************
 New unit test framework
 Features:
  - automatic test registration in test framework
  - unit test are placed in separate cpp file

 How to use
    DAVA_TESTCLASS(my_unittest)
    {
        my_unittest();
        ~my_unittest();

        void SetUp(const String& testName) override;
        void TearDown(const String& testName) override;
        void Update(float32 timeElapsed, const String& testName) override;
        bool TestComplete(const String& testName) const override;

        DAVA_TEST(test1)
        {
            TEST_VERIFY(0);
        }
        DAVA_TEST(test2)
        {
            TEST_VERIFY_WITH_MESSAGE(0, "my message");
        }
    };

 DAVA_TESTCLASS defines unit test class with name 'my_unittest'. This class has two tests: test1 and test2.
 Tests test1 and test2 will be executed by test framework in order of declaration. Inside tests you can
 verify assertions with TEST_VERIFY or TEST_VERIFY_WITH_MESSAGE. TEST_VERIFY_WITH_MESSAGE allows append user
 message to output when assertion fails. TEST_VERIFY and TEST_VERIFY_WITH_MESSAGE also can be invoked from
 functions that are called from tests.

 As DAVA_TESTCLASS declares C++ class you can define you own data and function members.

 You can define optional constructor and destructor for test class. Constructor should be defined with no
 parameters or only with default parameters.
 Also you are allowed to define optional member functions SetUp(), TearDown(), Update() and TestComplete().
 They take as argument current executing test name (test1 or test2 in example).

 SetUp() is invoked before running test, TearDown() is called after test has finished.
 You can override TestComplete() method to control test completion youself. Method Update() is invoked each frame
 while TestComplete() returns false.

 Lifetime of unittest and function order:
    my_unittest* unittest = new my_unittest;
    unittest->SetUp("test1");
    while (!unittest->TestComplete("test1"))
       unittest->Update(timeElapsed, "test1");
    unittest->TearDown("test1");

    same sequence for test2

    delete unittest;

 ==============================================================================================================

 Also you can declare custom base class for your test class using DAVA_TESTCLASS_CUSTOM_BASE macro. Base class should be derived from
 DAVA::UnitTests::TestClass and have default constructor.

 class MyBaseTestClass : public DAVA::UnitTests::TestClass
 {
 public:
    int CalcSomethingUseful(int param) { return 1 * 42; }
 };

 DAVA_TESTCLASS_CUSTOM_BASE(MyTestClass, MyBaseTestClass)
 {
    DAVA_TEST(Test1)
    {
        TEST_VERIFY(CalcSomethingUseful(1) == 42);
    }
 };

 ==============================================================================================================

 If you need make some additional initialization for custom BaseTestClass, you can implement metaIndex factory for your test classes and use
 DAVA_TESTCLASS_CUSTOM_BASE_AND_FACTORY macro to declare it. Custom BaseTestClass should be derived from DAVA::UnitTests::TestClass.
 Custom factory should be template class and derived from DAVA::UnitTests::TestClassFactoryBase.

 class MyBaseTestClass : public DAVA::UnitTests::TestClass
 {
 public:
 void Init() { do some additional initialization }
 int CalcSomethingUseful(int param) { return param * 42; }
 };

 template <typename T>
 class MyCustomTestClassFactory : public DAVA::UnitTests::TestClassFactoryBase
 {
 public:
     TestClass* CreateTestClass() override
     {
        T* testClass = new T();
        testClass->Init();
        return testClass;
     }
 }

 DAVA_TESTCLASS_CUSTOM_BASE_AND_FACTORY(MyTestClass, MyBaseTestClass, MyCustomTestClassFactory)
 {
 DAVA_TEST(Test1)
 {
 TEST_VERIFY(CalcSomethingUseful(1) == 42);
 }
 };

 ==============================================================================================================

 To run unit tests you should do some initialization:
 1. setup callbacks which will be called on test start, error, etc
    Supported callbacks are:
        void TestClassStartedCallback(const String& testClassName) - notifies that new test class will be prepared for running its tests
        void TestClassFinishedCallback(const String& testClassName) - notifies that test class's tests have been complete
        void TestClassDisabledCallback(const String& testClassName) - notifies that test class is disabled;
        void TestStartedCallback(const String& testClassName, const String& testName) - notifies about starting test run
        void TestFinishedCallback(const String& testClassName, const String& testName) - notifies that test has been finished
        void TestFailedCallback(const String& testClassName, const String& testName, const String& condition,
                                const char* filename, int line, const String& userMessage) - notifies about test failing
    Callback are set up through DAVA::UnitTests::TestCore::Instance()->Init() call.
 2. optionally specify list of test class names that only should be run
        DAVA::UnitTests::TestCore::Instance()->RunOnlyTheseTestClasses() - takes string of semicolon separated names
 3. optionally specify list of test class names that shouldn't run
        DAVA::UnitTests::TestCore::Instance()->DisableTheseTestClasses() - takes string of semicolon separated names
 4. you can optionally check number of registered test classes
        DAVA::UnitTests::TestCore::Instance()->HasTestClasses()

 Now test framework is ready to run tests. You should periodically call DAVA::UnitTests::TestCore::Instance()->ProcessTests() until it returns false:
    void GameCore::Update(float32 timeElapsed)
    {
       if (!UnitTests::TestCore::Instance()->ProcessTests(timeElapsed))
       {
           // all tests has been finished
       }
    }

******************************************************************************************/

#define DAVA_TESTCLASS(classname)                                                                                                               \
    struct classname;                                                                                                                           \
    static struct testclass_##classname##_registrar                                                                                         \
    {                                                                                                                                           \
        testclass_##classname##_registrar()                                                                                                 \
        {                                                                                                                                       \
            DAVA::UnitTests::TestCore::Instance()->RegisterTestClass(#classname, new DAVA::UnitTests::TestClassFactoryImpl<classname>);         \
        }                                                                                                                                       \
    } testclass_##classname##_registrar_instance;                                                                                           \
    struct classname : public DAVA::UnitTests::TestClass, public DAVA::UnitTests::TestClassTypeKeeper<classname>

#define DAVA_TESTCLASS_CUSTOM_BASE(classname, base_classname)                                                                                   \
    struct classname;                                                                                                                           \
    static struct testclass_##classname##_registrar                                                                                         \
    {                                                                                                                                           \
        testclass_##classname##_registrar()                                                                                                 \
        {                                                                                                                                       \
            DAVA::UnitTests::TestCore::Instance()->RegisterTestClass(#classname, new DAVA::UnitTests::TestClassFactoryImpl<classname>);         \
        }                                                                                                                                       \
    } testclass_##classname##_registrar_instance;                                                                                           \
    struct classname : public base_classname, public DAVA::UnitTests::TestClassTypeKeeper<classname>

#define DAVA_TESTCLASS_CUSTOM_BASE_AND_FACTORY(classname, base_classname, factory)                                                           \
    struct classname;                                                                                                                       \
    static struct testclass_##classname##_registrar                                                                                         \
    {                                                                                                                                       \
        testclass_##classname##_registrar()                                                                                                 \
        {                                                                                                                                   \
            DAVA::UnitTests::TestCore::Instance()->RegisterTestClass(#classname, new factory<classname>);                                   \
        }                                                                                                                                   \
    } testclass_##classname##_registrar_instance;                                                                                           \
    struct classname : public base_classname, public DAVA::UnitTests::TestClassTypeKeeper<classname>

#define DAVA_TEST(testname)                                                                                             \
    struct test_##testname##_registrar {                                                                            \
        test_##testname##_registrar(DAVA::UnitTests::TestClass* testClass)                                          \
        {                                                                                                               \
            testClass->RegisterTest(#testname, &test_##testname##_call);                                            \
        }                                                                                                               \
    };                                                                                                                  \
    test_##testname##_registrar test_##testname##_registrar_instance = test_##testname##_registrar(this);   \
    static void test_##testname##_call(DAVA::UnitTests::TestClass* testClass)                                       \
    {                                                                                                                   \
        static_cast<TestClassType*>(testClass)->testname();                                                             \
    }                                                                                                                   \
    void testname()

#define TEST_VERIFY_WITH_MESSAGE(condition, message)                                                                            \
    if (!(condition))                                                                                                           \
    {                                                                                                                           \
        DAVA::UnitTests::TestCore::Instance()->TestFailed(DAVA::String(#condition), __FILE__, __LINE__, DAVA::String(message)); \
    }

#define TEST_VERIFY(condition) TEST_VERIFY_WITH_MESSAGE(condition, DAVA::String())

//////////////////////////////////////////////////////////////////////////
// Macros that declare source files that are covered by unit test
//
// Usage:
//  DAVA_TESTCLASS(UsefulTest)
//  {
//      BEGIN_FILES_COVERED_BY_TESTS()
//          DECLARE_COVERED_FILES("FileSystem.cpp")
//          DECLARE_COVERED_FILES("JobManager.cpp")
//      FIND_FILES_IN_TARGET( DavaTools )
//          DECLARE_COVERED_FILES("FramePathHelper.cpp")
//      END_FILES_COVERED_BY_TESTS()
//
//      DAVA_TEST(test1) {}
//  };
//
// Test class UsefulTest covers two file: "FileSystem.cpp" and "JobManager.cpp"
//
// FIND_FILES_IN_TARGET( TARGET_NAME )
// Explicitly tells that next files belong to specified target. It is used to distinguish
// files with the same names located in different targets
//
// or to automatically deduce covered file from test class name
//  DAVA_TESTCLASS(DateTimeTest)
//  {
//      DEDUCE_COVERED_FILES_FROM_TESTCLASS()
//  };
//
// DEDUCE_COVERED_FILES_FROM_TESTCLASS discards Test postfix of any and considers that
// DateTimeTest covers class DateTime.
//
//
// This is test author's responsibility to specify valid and corresponding classes
//
// You can get and process classes covered by tests through call to DAVA::UnitTests::TestCore::Instance()->GetTestCoverage()
// which returns Map<String, Vector<String>> where key is test class name and value is vector of covered files
//
//Macros DAVA_FOLDERS, TARGET_FOLDERS are installed by cmake through add_definitions(...)
//DAVA_FOLDERS general list of folders where the sources are
//TARGET_FOLDERS list of folders with source code for the current target has
//

#if defined(TEST_COVERAGE)

#define BEGIN_FILES_COVERED_BY_TESTS() \
    DAVA::UnitTests::TestCoverageInfo FilesCoveredByTests() const override { \
        DAVA::UnitTests::TestCoverageInfo testInfo;  \
        testInfo.targetFolders.emplace("all", DAVA::String(DAVA_FOLDERS)); \
        const char* targetFolders = nullptr;

#define FIND_FILES_IN_TARGET(targetname) \
        targetFolders = TARGET_FOLDERS_##targetname;

#define DECLARE_COVERED_FILES(classname) \
        testInfo.testFiles.emplace_back(classname); \
        if (targetFolders != nullptr)\
        { \
            testInfo.targetFolders.emplace(DAVA::String(classname), DAVA::String(targetFolders));\
        }

#define END_FILES_COVERED_BY_TESTS() \
        return testInfo; \
    }

#define DEDUCE_COVERED_FILES_FROM_TESTCLASS() \
        BEGIN_FILES_COVERED_BY_TESTS() \
            DECLARE_COVERED_FILES(PrettifyTypeName(DAVA::String(typeid(*this).name())) + DAVA::String(".cpp")) \
        END_FILES_COVERED_BY_TESTS()

#else

#define BEGIN_FILES_COVERED_BY_TESTS()
#define FIND_FILES_IN_TARGET(targetname)
#define DECLARE_COVERED_FILES(classname)
#define END_FILES_COVERED_BY_TESTS()
#define DEDUCE_COVERED_FILES_FROM_TESTCLASS()

#endif

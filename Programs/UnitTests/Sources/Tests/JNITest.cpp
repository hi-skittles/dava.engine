#include "UnitTests/UnitTests.h"

#if defined(__DAVAENGINE_ANDROID__)

#include "Base/BaseTypes.h"
#include "Base/RefPtr.h"
#include "Concurrency/Thread.h"
#include "Engine/Engine.h"
#include "Engine/PlatformApiAndroid.h"
#include "Utils/StringFormat.h"

#include "Logger/Logger.h"

using namespace DAVA;

DAVA_DECLARE_CUSTOM_JNI_TYPE(junitTests, jobject, "Lcom/dava/unittests/UnitTests;");
DAVA_DECLARE_CUSTOM_JNI_TYPE(jtestObject, jobject, "Lcom/dava/unittests/TestObject;");

namespace
{
int nativeCallPassCount = 0;
}

extern "C"
{
JNIEXPORT void JNICALL Java_com_dava_unittests_UnitTests_nativeCall(JNIEnv* env, jobject classthis)
{
    nativeCallPassCount += 1;
}
}

DAVA_TESTCLASS (JNITest)
{
    void PlayWithJni()
    {
        Function<junitTests()> instance;
        Function<jstring(jobject, jint, jint, jint)> sum;
        Function<jstringArray(jint, jint)> generateStringArray;
        Function<jstringArray(jobject, jstringArray)> updateStringArray;
        Function<void(jobject)> doNativeCall;

        Function<jtestObject()> ctor;
        Function<jtestObject(jint)> ctorWithParam;
        JNI::Field<jint> field;
        JNI::StaticField<jlong> staticField;

        try
        {
            // Intentionally limit scope of JavaClass as functions and fields should keep reference to jclass inside
            JNI::JavaClass unitTestsClass("com/dava/unittests/UnitTests");
            JNI::JavaClass testObjectClass("com/dava/unittests/TestObject");

            instance = unitTestsClass.GetStaticMethod<junitTests>("instance");
            generateStringArray = unitTestsClass.GetStaticMethod<jstringArray, jint, jint>("generateStringArray");
            sum = unitTestsClass.GetMethod<jstring, jint, jint, jint>("sum");
            updateStringArray = unitTestsClass.GetMethod<jstringArray, jstringArray>("updateStringArray");
            doNativeCall = unitTestsClass.GetMethod<void>("doNativeCall");

            ctor = testObjectClass.GetConstructor<jtestObject>();
            ctorWithParam = testObjectClass.GetConstructor<jtestObject, jint>();
            field = testObjectClass.GetField<jint>("field");
            staticField = testObjectClass.GetStaticField<jlong>("staticField");
        }
        catch (const JNI::Exception& e)
        {
            TEST_VERIFY_WITH_MESSAGE(false, e.what());
            return;
        }

        try
        {
            const int nstrings = 3;
            const int startIndex = 10;

            JNIEnv* env = JNI::GetEnv();
            JNI::LocalRef<junitTests> unitTestJavaObject = instance();

            {
                // Test simple java call
                JNI::LocalRef<jstring> js = sum(unitTestJavaObject, 1, 2, 3);
                String s = JNI::JavaStringToString(js, env);
                TEST_VERIFY(s == "6");
            }
            {
                // Test arrays
                JNI::LocalRef<jstringArray> strings = generateStringArray(nstrings, startIndex);
                for (int i = 0; i < nstrings; ++i)
                {
                    JNI::LocalRef<jstring> js = env->GetObjectArrayElement(strings, i);
                    JNI::CheckJavaException(env, true);

                    String x = Format("%d", i + startIndex);
                    String s = JNI::JavaStringToString(js, env);
                    TEST_VERIFY(x == s);
                }

                strings = updateStringArray(unitTestJavaObject, strings);
                for (int i = 0; i < nstrings; ++i)
                {
                    JNI::LocalRef<jstring> js = env->GetObjectArrayElement(strings, i);
                    JNI::CheckJavaException(env, true);

                    String x = Format("%d-xray", i + startIndex);
                    String s = JNI::JavaStringToString(js, env);
                    TEST_VERIFY(x == s);
                }
            }
            {
                // Test native calls
                int n = nativeCallPassCount;
                doNativeCall(unitTestJavaObject);
                TEST_VERIFY(nativeCallPassCount == n + 1);
            }

            {
                // Test constructors and fields
                TEST_VERIFY(staticField.Get() == 0);
                JNI::LocalRef<jtestObject> instance1 = ctor();
                TEST_VERIFY(staticField.Get() == 1);
                JNI::LocalRef<jtestObject> instance2 = ctorWithParam(22);
                TEST_VERIFY(staticField.Get() == 2);

                TEST_VERIFY(field.Get(instance1) == 0);
                TEST_VERIFY(field.Get(instance2) == 22);

                field.Set(instance1, 111);
                field.Set(instance2, 333);

                TEST_VERIFY(field.Get(instance1) == 111);
                TEST_VERIFY(field.Get(instance2) == 333);

                staticField.Set(777);
                TEST_VERIFY(staticField.Get() == 777);
                staticField.Set(0);
            }
        }
        catch (const JNI::Exception& e)
        {
            TEST_VERIFY_WITH_MESSAGE(false, e.what());
            return;
        }
    }

    DAVA_TEST (TestLocalRefDelete)
    {
        // Test whether local references are automatically deleted.
        // By default each local JNI frame has local reference table with capacity of 512 elements,
        // if local reference count exceeds that capacity application crashes with message like this:
        // "JNI ERROR (app bug): local reference table overflow (max=512)"
        try
        {
            for (int i = 0; i < 1024; ++i)
            {
                JNI::JavaClass testObjectClass("java/lang/String");
                Function<jstring()> ctor = testObjectClass.GetConstructor<jstring>();
                JNI::LocalRef<jstring> obj = ctor();
            }
        }
        catch (const JNI::Exception& e)
        {
            TEST_VERIFY_WITH_MESSAGE(false, e.what());
            return;
        }
    }

    DAVA_TEST (PlayWithJniFromMainThread)
    {
        PlayWithJni();
    }

    RefPtr<Thread> foreignThread;
    bool foreignThreadFinished = false;

    DAVA_TEST (PlayWithJniFromForeignThread)
    {
        foreignThread = Thread::Create([this]() {
            PlayWithJni();
            RunOnMainThreadAsync([this]() {
                foreignThreadFinished = true;
            });
        });
        foreignThread->Start();
    }

    bool TestComplete(const String& testName) const override
    {
        if (testName == "PlayWithJniFromForeignThread")
            return foreignThreadFinished;
        return true;
    }
};

#endif // __DAVAENGINE_ANDROID__

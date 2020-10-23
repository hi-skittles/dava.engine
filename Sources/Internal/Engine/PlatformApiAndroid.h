#pragma once

/**
    \defgroup engine_android Engine facilities specific to Android and JNI wrappers
*/

#if defined(__DAVAENGINE_ANDROID__)

#include <jni.h>
#include <stdexcept>

#include "Base/BaseTypes.h"
#include "Debug/DVAssert.h"
#include "Engine/Private/Android/JNIDecl.h"
#include "Functional/Function.h"
#include "Math/Rect.h"
#include "Utils/StringFormat.h"

#define DAVA_DECLARE_CUSTOM_JNI_TYPE(jnitype, base, signature) \
    class dava_custom_jni_type##jnitype : public std::remove_pointer<base>::type {}; \
    using jnitype = dava_custom_jni_type##jnitype*; \
    template <> struct DAVA::JNI::TypeSignature<jnitype> { static const DAVA::char8* value() { return signature; } }

DAVA_DECLARE_CUSTOM_JNI_TYPE(jstringArray, jobjectArray, "[Ljava/lang/String;");

#define DAVA_JNI_EXCEPTION_CHECK() \
    do { \
        try { \
            JNIEnv* env = DAVA::JNI::GetEnv(); \
            DAVA::JNI::CheckJavaException(env, true); \
        } catch (const DAVA::JNI::Exception& e) { \
            DVASSERT(false, e.what()); \
        } \
    } while (0)

namespace DAVA
{
class Window;
namespace PlatformApi
{
namespace Android
{
jobject CreateNativeControl(Window* targetWindow, const char8* controlClassName, void* backendPointer);

} // namespace Android
} // namespace PlatformApi

namespace JNI
{
class Exception : public std::runtime_error
{
    using std::runtime_error::runtime_error;
};

JNIEnv* GetEnv(bool abortIfNotAttachedToJVM = true);
void AttachCurrentThreadToJVM();
void DetachCurrentThreadFromJVM();

bool CheckJavaException(JNIEnv* env, bool throwJniException = true);
String GetJavaExceptionText(JNIEnv* env, jthrowable e);

jclass LoadJavaClass(const char8* className, bool throwJniException = false, JNIEnv* env = nullptr);

String JavaStringToString(jstring string, JNIEnv* env = nullptr);
WideString JavaStringToWideString(jstring string, JNIEnv* env = nullptr);
jstring CStrToJavaString(const char* cstr, JNIEnv* env = nullptr);
jstring StringToJavaString(const String& string, JNIEnv* env = nullptr);
jstring WideStringToJavaString(const WideString& string, JNIEnv* env = nullptr);

// Functions left for compatibility
Rect V2I(const Rect& rect);
inline String ToString(const jstring jniString)
{
    return JavaStringToString(jniString);
}
inline WideString ToWideString(const jstring jniString)
{
    return JavaStringToWideString(jniString);
}
inline jstring ToJNIString(const DAVA::WideString& string)
{
    return WideStringToJavaString(string);
}

/**
    Template class `ObjectRef` is smart pointer like class that owns and manages JNI jobject and jobject-derived objects.

    Template parameters are:
        - T - jobject or jobject-derived type, also can be type registered through `DAVA_DECLARE_CUSTOM_JNI_TYPE` macro
        - NewRef - method of JNIEnv which creates new reference (can be `NewGlobalRef` or `NewLocalRef`)
        - DeleteRef - method of JNIEnv which deletes reference (can be `DeleteGlobalRef` or `DeleteLocalRef`)

    `ObjectRef` is not intended for direct use, instead use `GlobalRef<T>` alias for global references and `LocalRef<T>` for local references.

    Rules when creating ObjectRef instance or assigning with raw jobject:
    1. if ObjectRef is GlobalRef:
        if jobject is global reference then GlobalRef takes ownership of jobject
        if jobject is local reference then GlobalRef creates and manages global reference and delete jobject
    2. if ObjectRef is LocalRef:
        if jobject is global reference then LocalRef creates and manages local reference
        if jobject is local reference then LocalRef takes ownership of jobject

    Samples:
    \code
        GlobalRef<jstring> g1; // Some global jstring object
        LocalRef<jobject> l1 = ...; // Usually java methods return local references to jobjects,
                                    // l1 will take ownership of returned jobject
        GlobalRef<jobject> g8 = ...; // Java method returns local reference and g8 creates and manages global
                                     // reference and deletes returned local reference
        g1 = l1; // Convert and create new global reference to jstring, local reference will be automatically deleted

        LocalRef<jobject> l2 = ...;
        GlobalRef<jstring> g2(std::move(l2)); // Convert and create new global reference to jstring, automatically deleting local reference
    \endcode
*/
template <typename T, jobject (JNIEnv::*NewRef)(jobject), void (JNIEnv::*DeleteRef)(jobject)>
struct ObjectRef
{
    // clang-format off
    static_assert(std::is_base_of<std::remove_pointer_t<jobject>, std::remove_pointer_t<T>>::value, "T must be jobject or jobject-based type");
    static_assert(
        (NewRef == &JNIEnv::NewGlobalRef && DeleteRef == &JNIEnv::DeleteGlobalRef) ||
        (NewRef == &JNIEnv::NewLocalRef && DeleteRef == &JNIEnv::DeleteLocalRef),
        "NewRef and DeleteRef must be consistent");
    // clang-format on

    using Type = T; //<! T, the jobject-related type managed by this `ObjectRef`

    static const bool isGlobalRef = NewRef == &JNIEnv::NewGlobalRef && DeleteRef == &JNIEnv::DeleteGlobalRef;

    ObjectRef() = default;

    /** Constructor that takes ownership of object that is the same type as T. */
    ObjectRef(T obj)
        : object(Assign(obj))
    {
    }

    /** Constructor that takes ownership of object that is the convertible to T. */
    template <typename Other>
    ObjectRef(Other obj)
        : ObjectRef(static_cast<T>(obj))
    {
    }

    /** Constructor that creates new reference to object managed by `other`. */
    ObjectRef(const ObjectRef& other)
        : object(Retain(other.object))
    {
    }

    /** Constructor that creates new reference to object managed by `other`. */
    template <typename Other, jobject (JNIEnv::*NewRefOther)(jobject), void (JNIEnv::*DeleteRefOther)(jobject)>
    ObjectRef(const ObjectRef<Other, NewRefOther, DeleteRefOther>& other)
        : object(Retain(other.object))
    {
    }

    ObjectRef& operator=(const ObjectRef& other)
    {
        if (this != &other)
        {
            Release();
            object = Retain(other.object);
        }
        return *this;
    }

    template <typename Other, jobject (JNIEnv::*NewRefOther)(jobject), void (JNIEnv::*DeleteRefOther)(jobject)>
    ObjectRef& operator=(const ObjectRef<Other, NewRefOther, DeleteRefOther>& other)
    {
        Release();
        object = Retain(other.object);
        return *this;
    }

    ObjectRef(ObjectRef&& other)
        : object(Retain(other.object))
    {
        other.Release();
    }

    template <typename Other, jobject (JNIEnv::*NewRefOther)(jobject), void (JNIEnv::*DeleteRefOther)(jobject)>
    ObjectRef(ObjectRef<Other, NewRefOther, DeleteRefOther>&& other)
        : object(Retain(other.object))
    {
        other.Release();
    }

    ObjectRef& operator=(ObjectRef&& other)
    {
        if (this != &other)
        {
            Release();
            object = Retain(other.object);
            other.Release();
        }
        return *this;
    }

    template <typename Other, jobject (JNIEnv::*NewRefOther)(jobject), void (JNIEnv::*DeleteRefOther)(jobject)>
    ObjectRef& operator=(ObjectRef<Other, NewRefOther, DeleteRefOther>&& other)
    {
        Release();
        object = Retain(other.object);
        other.Release();
        return *this;
    }

    ~ObjectRef()
    {
        Release();
    }

    T Get() const
    {
        return object;
    }

    ObjectRef& operator=(T obj)
    {
        if (object != obj)
        {
            Release();
            object = Assign(obj);
        }
        return *this;
    }

    template <typename Other>
    ObjectRef& operator=(Other obj)
    {
        return operator=(static_cast<T>(obj));
    }

    operator T() const
    {
        return object;
    }

private:
    T Assign(T obj)
    {
        T result = nullptr;
        if (obj != nullptr)
        {
            jobjectRefType otherType = GetEnv()->GetObjectRefType(obj);
            DVASSERT(otherType == JNILocalRefType || otherType == JNIGlobalRefType);
            if (isGlobalRef)
            {
                if (otherType == JNILocalRefType)
                {
                    result = Retain(obj);
                    GetEnv()->DeleteLocalRef(obj);
                }
                else if (otherType == JNIGlobalRefType)
                {
                    result = obj;
                }
            }
            else
            {
                if (otherType == JNILocalRefType)
                {
                    result = obj;
                }
                else if (otherType == JNIGlobalRefType)
                {
                    result = Retain(obj);
                }
            }
        }
        return result;
    }

    template <typename U>
    T Retain(U obj)
    {
        T result = nullptr;
        if (obj != nullptr)
        {
            result = static_cast<T>((GetEnv()->*NewRef)(obj));
        }
        return result;
    }
    void Release()
    {
        if (object != nullptr)
        {
            (GetEnv()->*DeleteRef)(object);
            object = nullptr;
        }
    }

    T object = nullptr;

    template <typename, jobject (JNIEnv::*)(jobject), void (JNIEnv::*)(jobject)>
    friend class ObjectRef;
};

template <typename T, jobject (JNIEnv::*NewRef)(jobject), void (JNIEnv::*DeleteRef)(jobject)>
bool operator==(const ObjectRef<T, NewRef, DeleteRef>& o, std::nullptr_t)
{
    return o.Get() == nullptr;
}

template <typename T, jobject (JNIEnv::*NewRef)(jobject), void (JNIEnv::*DeleteRef)(jobject)>
bool operator==(std::nullptr_t, const ObjectRef<T, NewRef, DeleteRef>& o)
{
    return o.Get() == nullptr;
}

template <typename T, jobject (JNIEnv::*NewRef)(jobject), void (JNIEnv::*DeleteRef)(jobject)>
bool operator!=(const ObjectRef<T, NewRef, DeleteRef>& o, std::nullptr_t)
{
    return !(o == nullptr);
}

template <typename T, jobject (JNIEnv::*NewRef)(jobject), void (JNIEnv::*DeleteRef)(jobject)>
bool operator!=(std::nullptr_t, const ObjectRef<T, NewRef, DeleteRef>& o)
{
    return !(o == nullptr);
}

template <typename T>
using GlobalRef = ObjectRef<T, &JNIEnv::NewGlobalRef, &JNIEnv::DeleteGlobalRef>;
template <typename T>
using LocalRef = ObjectRef<T, &JNIEnv::NewLocalRef, &JNIEnv::DeleteLocalRef>;

template <typename R>
class Field
{
public:
    Field() = default;

    R Get(jobject obj) const
    {
        JNIEnv* env = GetEnv();
        R r = (env->*TypedMethod<R>::GetField)(obj, fieldID);
        CheckJavaException(env, true);
        return r;
    }

    void Set(jobject obj, R value)
    {
        JNIEnv* env = GetEnv();
        (env->*TypedMethod<R>::SetField)(obj, fieldID, value);
        CheckJavaException(env, true);
    }

private:
    Field(jfieldID f)
        : fieldID(f)
    {
    }

    jfieldID fieldID = nullptr;

    friend class JavaClass;
};

template <typename R>
class StaticField
{
public:
    StaticField() = default;

    R Get() const
    {
        JNIEnv* env = GetEnv();
        R r = (env->*TypedMethod<R>::GetStaticField)(clazz, fieldID);
        CheckJavaException(env, true);
        return r;
    }

    void Set(R value)
    {
        JNIEnv* env = GetEnv();
        (env->*TypedMethod<R>::SetStaticField)(clazz, fieldID, value);
        CheckJavaException(env, true);
    }

private:
    StaticField(const GlobalRef<jclass>& c, jfieldID f)
        : clazz(c)
        , fieldID(f)
    {
    }

    GlobalRef<jclass> clazz;
    jfieldID fieldID = nullptr;

    friend class JavaClass;
};

class JavaClass
{
public:
    JavaClass() = default;
    JavaClass(const char8* className)
        : clazz(LoadJavaClass(className, true))
    {
    }
    JavaClass(const String& className)
        : JavaClass(className.c_str())
    {
    }

    JavaClass(const JavaClass& other) = default;
    JavaClass& operator=(const JavaClass& other) = default;

    JavaClass(JavaClass&& other) = default;
    JavaClass& operator=(JavaClass&& other) = default;

    ~JavaClass() = default;

    operator jclass() const;

    template <typename R, typename... Args>
    Function<R(Args...)> GetConstructor() const;

    template <typename R, typename... Args>
    Function<R(jobject, Args...)> GetMethod(const char8* name) const;

    template <typename R, typename... Args>
    Function<R(Args...)> GetStaticMethod(const char8* name) const;

    template <typename R>
    Field<R> GetField(const char8* name) const;

    template <typename R>
    StaticField<R> GetStaticField(const char8* name) const;

private:
    template <typename R, typename... Args>
    struct ConstructorCaller
    {
        ConstructorCaller(const GlobalRef<jclass>& c, jmethodID m)
            : clazz(c)
            , methodID(m)
        {
        }
        R operator()(Args... args) const
        {
            JNIEnv* env = GetEnv();
            jobject r = env->NewObject(clazz, methodID, args...);
            CheckJavaException(env, true);
            return static_cast<R>(r);
        }
        GlobalRef<jclass> clazz;
        jmethodID methodID = nullptr;
    };

    template <typename R, typename... Args>
    struct MethodCaller
    {
        MethodCaller(jmethodID m)
            : methodID(m)
        {
        }
        R operator()(jobject object, Args... args) const
        {
            JNIEnv* env = GetEnv();
            R r = static_cast<R>((env->*TypedMethod<R>::Call)(object, methodID, args...));
            CheckJavaException(env, true);
            return r;
        }
        jmethodID methodID = nullptr;
    };

    template <typename... Args>
    struct MethodCaller<void, Args...>
    {
        MethodCaller(jmethodID m)
            : methodID(m)
        {
        }
        void operator()(jobject object, Args... args) const
        {
            JNIEnv* env = GetEnv();
            (env->*TypedMethod<void>::Call)(object, methodID, args...);
            CheckJavaException(env, true);
        }
        jmethodID methodID = nullptr;
    };

    template <typename R, typename... Args>
    struct StaticMethodCaller
    {
        StaticMethodCaller(const GlobalRef<jclass>& c, jmethodID m)
            : clazz(c)
            , methodID(m)
        {
        }
        R operator()(Args... args) const
        {
            JNIEnv* env = GetEnv();
            R r = static_cast<R>((env->*TypedMethod<R>::CallStatic)(clazz, methodID, args...));
            CheckJavaException(env, true);
            return r;
        }
        GlobalRef<jclass> clazz;
        jmethodID methodID = nullptr;
    };

    template <typename... Args>
    struct StaticMethodCaller<void, Args...>
    {
        StaticMethodCaller(const GlobalRef<jclass>& c, jmethodID m)
            : clazz(c)
            , methodID(m)
        {
        }
        void operator()(Args... args) const
        {
            JNIEnv* env = GetEnv();
            (env->*TypedMethod<void>::CallStatic)(clazz, methodID, args...);
            CheckJavaException(env, true);
        }
        GlobalRef<jclass> clazz;
        jmethodID methodID = nullptr;
    };

private:
    GlobalRef<jclass> clazz;
};

inline JavaClass::operator jclass() const
{
    return clazz;
}

template <typename R, typename... Args>
Function<R(Args...)> JavaClass::GetConstructor() const
{
    static_assert(std::is_base_of<std::remove_pointer_t<jobject>, std::remove_pointer_t<R>>::value, "T must be jobject or jobject-based type");
    JNIEnv* env = GetEnv();
    jmethodID method = env->GetMethodID(clazz, "<init>", TypeSignature<void(Args...)>::value());
    CheckJavaException(env, true);
    return Function<R(Args...)>(ConstructorCaller<R, Args...>(clazz, method));
}

template <typename R, typename... Args>
Function<R(jobject, Args...)> JavaClass::GetMethod(const char8* name) const
{
    JNIEnv* env = GetEnv();
    jmethodID method = env->GetMethodID(clazz, name, TypeSignature<R(Args...)>::value());
    CheckJavaException(env, true);
    return Function<R(jobject, Args...)>(MethodCaller<R, Args...>(method));
}

template <typename R, typename... Args>
Function<R(Args...)> JavaClass::GetStaticMethod(const char8* name) const
{
    JNIEnv* env = GetEnv();
    jmethodID method = env->GetStaticMethodID(clazz, name, TypeSignature<R(Args...)>::value());
    CheckJavaException(env, true);
    return Function<R(Args...)>(StaticMethodCaller<R, Args...>(clazz, method));
}

template <typename R>
Field<R> JavaClass::GetField(const char8* name) const
{
    JNIEnv* env = GetEnv();
    jfieldID field = env->GetFieldID(clazz, name, TypeSignature<R>::value());
    CheckJavaException(env, true);
    return Field<R>(field);
}

template <typename R>
StaticField<R> JavaClass::GetStaticField(const char8* name) const
{
    JNIEnv* env = GetEnv();
    jfieldID field = env->GetStaticFieldID(clazz, name, TypeSignature<R>::value());
    CheckJavaException(env, true);
    return StaticField<R>(clazz, field);
}

} // namespace JNI
} // namespace DAVA

#endif // __DAVAENGINE_ANDROID__

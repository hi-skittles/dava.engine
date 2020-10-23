#pragma once

#if defined(__DAVAENGINE_ANDROID__)

namespace DAVA
{
namespace JNI
{
////////  TypeSignature  /////////////////////////////////////////////////
template <typename T, typename = void>
struct TypeSignature;

template <>
struct TypeSignature<void>
{
    static const char8* value()
    {
        return "V";
    }
};
template <>
struct TypeSignature<jboolean>
{
    static const char8* value()
    {
        return "Z";
    }
};
template <>
struct TypeSignature<jbyte>
{
    static const char8* value()
    {
        return "B";
    }
};
template <>
struct TypeSignature<jchar>
{
    static const char8* value()
    {
        return "C";
    }
};
template <>
struct TypeSignature<jshort>
{
    static const char8* value()
    {
        return "S";
    }
};
template <>
struct TypeSignature<jint>
{
    static const char8* value()
    {
        return "I";
    }
};
template <>
struct TypeSignature<jlong>
{
    static const char8* value()
    {
        return "J";
    }
};
template <>
struct TypeSignature<jfloat>
{
    static const char8* value()
    {
        return "F";
    }
};
template <>
struct TypeSignature<jdouble>
{
    static const char8* value()
    {
        return "D";
    }
};

template <>
struct TypeSignature<jobject>
{
    static const char8* value()
    {
        return "Ljava/lang/Object;";
    }
};
template <>
struct TypeSignature<jstring>
{
    static const char8* value()
    {
        return "Ljava/lang/String;";
    }
};
template <>
struct TypeSignature<jarray>
{
    static const char8* value()
    {
        return "Ljava/lang/Object;";
    }
};
template <>
struct TypeSignature<jobjectArray>
{
    static const char8* value()
    {
        return "[Ljava/lang/Object;";
    }
};
template <>
struct TypeSignature<jbooleanArray>
{
    static const char8* value()
    {
        return "[Z";
    }
};
template <>
struct TypeSignature<jbyteArray>
{
    static const char8* value()
    {
        return "[B";
    }
};
template <>
struct TypeSignature<jcharArray>
{
    static const char8* value()
    {
        return "[C";
    }
};
template <>
struct TypeSignature<jshortArray>
{
    static const char8* value()
    {
        return "[S";
    }
};
template <>
struct TypeSignature<jintArray>
{
    static const char8* value()
    {
        return "[I";
    }
};
template <>
struct TypeSignature<jlongArray>
{
    static const char8* value()
    {
        return "[J";
    }
};
template <>
struct TypeSignature<jfloatArray>
{
    static const char8* value()
    {
        return "[F";
    }
};
template <>
struct TypeSignature<jdoubleArray>
{
    static const char8* value()
    {
        return "[D";
    }
};

template <typename...>
struct BuildTypeSignature;

template <typename T, typename... Tail>
struct BuildTypeSignature<T, Tail...>
{
    static String value(String s)
    {
        s += TypeSignature<T>::value();
        s = BuildTypeSignature<Tail...>::value(s);
        return s;
    }
};

template <>
struct BuildTypeSignature<>
{
    static String value(String s)
    {
        return s;
    }
};

template <typename R, typename... Args>
struct TypeSignature<R(Args...)>
{
    static const char8* value()
    {
        static String v(String("(") + BuildTypeSignature<Args...>::value(String()) + String(")") + TypeSignature<R>::value());
        return v.c_str();
    }
};

////////  TypedMethod  ///////////////////////////////////////////////////
template <typename R, typename = void>
struct TypedMethod;

template <typename R>
struct TypedMethod<R, std::enable_if_t<std::is_base_of<std::remove_pointer_t<jobject>, std::remove_pointer_t<R>>::value>>
{
    static constexpr auto Call = &JNIEnv::CallObjectMethod;
    static constexpr auto CallStatic = &JNIEnv::CallStaticObjectMethod;
    static constexpr auto GetField = &JNIEnv::GetObjectField;
    static constexpr auto SetField = &JNIEnv::SetObjectField;
    static constexpr auto GetStaticField = &JNIEnv::GetStaticObjectField;
    static constexpr auto SetStaticField = &JNIEnv::SetStaticObjectField;
};

template <>
struct TypedMethod<void>
{
    static constexpr auto Call = &JNIEnv::CallVoidMethod;
    static constexpr auto CallStatic = &JNIEnv::CallStaticVoidMethod;
};

template <>
struct TypedMethod<jboolean>
{
    static constexpr auto Call = &JNIEnv::CallBooleanMethod;
    static constexpr auto CallStatic = &JNIEnv::CallStaticBooleanMethod;
    static constexpr auto GetField = &JNIEnv::GetBooleanField;
    static constexpr auto SetField = &JNIEnv::SetBooleanField;
    static constexpr auto GetStaticField = &JNIEnv::GetStaticBooleanField;
    static constexpr auto SetStaticField = &JNIEnv::SetStaticBooleanField;
};

template <>
struct TypedMethod<jbyte>
{
    static constexpr auto Call = &JNIEnv::CallByteMethod;
    static constexpr auto CallStatic = &JNIEnv::CallStaticByteMethod;
    static constexpr auto GetField = &JNIEnv::GetByteField;
    static constexpr auto SetField = &JNIEnv::SetByteField;
    static constexpr auto GetStaticField = &JNIEnv::GetStaticByteField;
    static constexpr auto SetStaticField = &JNIEnv::SetStaticByteField;
};

template <>
struct TypedMethod<jchar>
{
    static constexpr auto Call = &JNIEnv::CallCharMethod;
    static constexpr auto CallStatic = &JNIEnv::CallStaticCharMethod;
    static constexpr auto GetField = &JNIEnv::GetCharField;
    static constexpr auto SetField = &JNIEnv::SetCharField;
    static constexpr auto GetStaticField = &JNIEnv::GetStaticCharField;
    static constexpr auto SetStaticField = &JNIEnv::SetStaticCharField;
};

template <>
struct TypedMethod<jshort>
{
    static constexpr auto Call = &JNIEnv::CallShortMethod;
    static constexpr auto CallStatic = &JNIEnv::CallStaticShortMethod;
    static constexpr auto GetField = &JNIEnv::GetShortField;
    static constexpr auto SetField = &JNIEnv::SetShortField;
    static constexpr auto GetStaticField = &JNIEnv::GetStaticShortField;
    static constexpr auto SetStaticField = &JNIEnv::SetStaticShortField;
};

template <>
struct TypedMethod<jint>
{
    static constexpr auto Call = &JNIEnv::CallIntMethod;
    static constexpr auto CallStatic = &JNIEnv::CallStaticIntMethod;
    static constexpr auto GetField = &JNIEnv::GetIntField;
    static constexpr auto SetField = &JNIEnv::SetIntField;
    static constexpr auto GetStaticField = &JNIEnv::GetStaticIntField;
    static constexpr auto SetStaticField = &JNIEnv::SetStaticIntField;
};

template <>
struct TypedMethod<jlong>
{
    static constexpr auto Call = &JNIEnv::CallLongMethod;
    static constexpr auto CallStatic = &JNIEnv::CallStaticLongMethod;
    static constexpr auto GetField = &JNIEnv::GetLongField;
    static constexpr auto SetField = &JNIEnv::SetLongField;
    static constexpr auto GetStaticField = &JNIEnv::GetStaticLongField;
    static constexpr auto SetStaticField = &JNIEnv::SetStaticLongField;
};

template <>
struct TypedMethod<jfloat>
{
    static constexpr auto Call = &JNIEnv::CallFloatMethod;
    static constexpr auto CallStatic = &JNIEnv::CallStaticFloatMethod;
    static constexpr auto GetField = &JNIEnv::GetFloatField;
    static constexpr auto SetField = &JNIEnv::SetFloatField;
    static constexpr auto GetStaticField = &JNIEnv::GetStaticFloatField;
    static constexpr auto SetStaticField = &JNIEnv::SetStaticFloatField;
};

template <>
struct TypedMethod<jdouble>
{
    static constexpr auto Call = &JNIEnv::CallDoubleMethod;
    static constexpr auto CallStatic = &JNIEnv::CallStaticDoubleMethod;
    static constexpr auto GetField = &JNIEnv::GetDoubleField;
    static constexpr auto SetField = &JNIEnv::SetDoubleField;
    static constexpr auto GetStaticField = &JNIEnv::GetStaticDoubleField;
    static constexpr auto SetStaticField = &JNIEnv::SetStaticDoubleField;
};

} // namespace JNI
} // namespace DAVA

#endif // __DAVAENGINE_ANDROID__

#include "Base/Platform.h"

#include "Base/BaseTypes.h"
#include "Engine/Engine.h"
#include "Engine/PlatformApiAndroid.h"
#include "Engine/Private/Android/AndroidBridge.h"
#include "Logger/Logger.h"

extern DAVA::Private::AndroidBridge* androidBridge;

namespace DAVA
{
namespace Debug
{
int MessageBox(const String& title, const String& message, const Vector<String>& buttons, int /*defaultButton*/)
{
    DVASSERT(0 < buttons.size() && buttons.size() <= 3);

    try
    {
        JNI::JavaClass msgboxClass("com/dava/engine/MessageBox");
        Function<jint(jstring, jstring, jstringArray)> showModal = msgboxClass.GetStaticMethod<jint, jstring, jstring, jstringArray>("messageBox");

        JNIEnv* env = JNI::GetEnv();
        JNI::LocalRef<jstring> jtitle = JNI::StringToJavaString(title, env);
        JNI::LocalRef<jstring> jmessage = JNI::StringToJavaString(message, env);

        JNI::GlobalRef<jclass> stringClass = JNI::LoadJavaClass("java/lang/String", true, env);
        jsize n = static_cast<jsize>(buttons.size());
        JNI::LocalRef<jstringArray> jbuttons = env->NewObjectArray(n, stringClass, nullptr);
        for (jsize i = 0; i < n; ++i)
        {
            JNI::LocalRef<jstring> jbuttonName = JNI::StringToJavaString(buttons[i], env);
            env->SetObjectArrayElement(jbuttons, i, jbuttonName);
            JNI::CheckJavaException(env, true);
        }
        return static_cast<int>(showModal(jtitle, jmessage, jbuttons));
    }
    catch (const JNI::Exception& e)
    {
        Logger::Error("MessageBox failed: %s", e.what());
        return -1;
    }
}

} // namespace Debug
} // namespace DAVA

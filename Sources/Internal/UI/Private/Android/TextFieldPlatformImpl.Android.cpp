#include "UI/Private/Android/TextFieldPlatformImpl.Android.h"

#if defined(__DAVAENGINE_ANDROID__)

#include "Engine/Engine.h"
#include "Engine/Window.h"

#include "Logger/Logger.h"
#include "Utils/UTF8Utils.h"
#include "UI/UITextField.h"
#include "UI/UITextFieldDelegate.h"
#include "UI/UIControlSystem.h"
#include "UI/Focus/FocusHelpers.h"
#include "Render/Image/Image.h"
#include "Render/Image/ImageConvert.h"
#include "Render/2D/Systems/VirtualCoordinatesSystem.h"
#include "UI/UIControlSystem.h"
#include "UI/UIControlBackground.h"
extern "C"
{

JNIEXPORT void JNICALL Java_com_dava_engine_DavaTextField_nativeReleaseWeakPtr(JNIEnv* env, jclass jclazz, jlong backendPointer)
{
    using DAVA::TextFieldPlatformImpl;

    // Postpone deleting in case some other jobs are posted to main thread
    DAVA::RunOnMainThreadAsync([backendPointer]() {
        std::weak_ptr<TextFieldPlatformImpl>* weak = reinterpret_cast<std::weak_ptr<TextFieldPlatformImpl>*>(static_cast<uintptr_t>(backendPointer));
        delete weak;
    });
}

JNIEXPORT void JNICALL Java_com_dava_engine_DavaTextField_nativeOnFocusChange(JNIEnv* env, jclass jclazz, jlong backendPointer, jboolean hasFocus)
{
    using DAVA::TextFieldPlatformImpl;
    std::weak_ptr<TextFieldPlatformImpl>* weak = reinterpret_cast<std::weak_ptr<TextFieldPlatformImpl>*>(static_cast<uintptr_t>(backendPointer));
    if (weak != nullptr)
    {
        if (auto backend = weak->lock())
            backend->nativeOnFocusChange(env, hasFocus);
    }
}

JNIEXPORT void JNICALL Java_com_dava_engine_DavaTextField_nativeOnKeyboardShown(JNIEnv* env, jclass jclazz, jlong backendPointer, jint x, jint y, jint w, jint h)
{
    using DAVA::TextFieldPlatformImpl;
    std::weak_ptr<TextFieldPlatformImpl>* weak = reinterpret_cast<std::weak_ptr<TextFieldPlatformImpl>*>(static_cast<uintptr_t>(backendPointer));
    if (weak != nullptr)
    {
        if (auto backend = weak->lock())
            backend->nativeOnKeyboardShown(env, x, y, w, h);
    }
}

JNIEXPORT void JNICALL Java_com_dava_engine_DavaTextField_nativeOnKeyboardHidden(JNIEnv* env, jclass jclazz, jlong backendPointer)
{
    using DAVA::TextFieldPlatformImpl;
    std::weak_ptr<TextFieldPlatformImpl>* weak = reinterpret_cast<std::weak_ptr<TextFieldPlatformImpl>*>(static_cast<uintptr_t>(backendPointer));
    if (weak != nullptr)
    {
        if (auto backend = weak->lock())
            backend->nativeOnKeyboardHidden(env);
    }
}

JNIEXPORT void JNICALL Java_com_dava_engine_DavaTextField_nativeOnEnterPressed(JNIEnv* env, jclass jclazz, jlong backendPointer)
{
    using DAVA::TextFieldPlatformImpl;
    std::weak_ptr<TextFieldPlatformImpl>* weak = reinterpret_cast<std::weak_ptr<TextFieldPlatformImpl>*>(static_cast<uintptr_t>(backendPointer));
    if (weak != nullptr)
    {
        if (auto backend = weak->lock())
            backend->nativeOnEnterPressed(env);
    }
}

JNIEXPORT jboolean JNICALL Java_com_dava_engine_DavaTextField_nativeOnKeyPressed(JNIEnv* env, jclass jclazz, jlong backendPointer, jint replacementStart, jint replacementLength, jstring replaceWith)
{
    using DAVA::TextFieldPlatformImpl;
    std::weak_ptr<TextFieldPlatformImpl>* weak = reinterpret_cast<std::weak_ptr<TextFieldPlatformImpl>*>(static_cast<uintptr_t>(backendPointer));
    if (weak != nullptr)
    {
        if (auto backend = weak->lock())
            return backend->nativeOnKeyPressed(env, replacementStart, replacementLength, replaceWith);
    }
    return JNI_TRUE;
}

JNIEXPORT void JNICALL Java_com_dava_engine_DavaTextField_nativeOnTextChanged(JNIEnv* env, jclass jclazz, jlong backendPointer, jstring newText, jboolean programmaticTextChange)
{
    using DAVA::TextFieldPlatformImpl;
    std::weak_ptr<TextFieldPlatformImpl>* weak = reinterpret_cast<std::weak_ptr<TextFieldPlatformImpl>*>(static_cast<uintptr_t>(backendPointer));
    if (weak != nullptr)
    {
        if (auto backend = weak->lock())
            backend->nativeOnTextChanged(env, newText, programmaticTextChange);
    }
}

JNIEXPORT void JNICALL Java_com_dava_engine_DavaTextField_nativeOnTextureReady(JNIEnv* env, jclass jclazz, jlong backendPointer, jintArray pixels, jint w, jint h)
{
    using DAVA::TextFieldPlatformImpl;
    std::weak_ptr<TextFieldPlatformImpl>* weak = reinterpret_cast<std::weak_ptr<TextFieldPlatformImpl>*>(static_cast<uintptr_t>(backendPointer));
    if (weak != nullptr)
    {
        if (auto backend = weak->lock())
            backend->nativeOnTextureReady(env, pixels, w, h);
    }
}
}

namespace DAVA
{
TextFieldPlatformImpl::TextFieldPlatformImpl(Window* w, UITextField* uiTextField)
    : window(w)
    , uiTextField(uiTextField)
{
}

TextFieldPlatformImpl::~TextFieldPlatformImpl() = default;

void TextFieldPlatformImpl::Initialize()
{
    try
    {
        textFieldJavaClass.reset(new JNI::JavaClass("com/dava/engine/DavaTextField"));
        release = textFieldJavaClass->GetMethod<void>("release");
        setVisible = textFieldJavaClass->GetMethod<void, jboolean>("setVisible");
        setIsPassword = textFieldJavaClass->GetMethod<void, jboolean>("setIsPassword");
        setMaxLength = textFieldJavaClass->GetMethod<void, jint>("setMaxLength");
        openKeyboard = textFieldJavaClass->GetMethod<void>("openKeyboard");
        closeKeyboard = textFieldJavaClass->GetMethod<void>("closeKeyboard");
        setRect = textFieldJavaClass->GetMethod<void, jfloat, jfloat, jfloat, jfloat>("setRect");
        setText = textFieldJavaClass->GetMethod<void, jstring>("setText");
        setTextColor = textFieldJavaClass->GetMethod<void, jint, jint, jint, jint>("setTextColor");
        setTextAlign = textFieldJavaClass->GetMethod<void, jint>("setTextAlign");
        setTextUseRtlAlign = textFieldJavaClass->GetMethod<void, jboolean>("setTextUseRtlAlign");
        setFontSize = textFieldJavaClass->GetMethod<void, jfloat>("setFontSize");
        setMultiline = textFieldJavaClass->GetMethod<void, jboolean>("setMultiline");
        setInputEnabled = textFieldJavaClass->GetMethod<void, jboolean>("setInputEnabled");
        setAutoCapitalizationType = textFieldJavaClass->GetMethod<void, jint>("setAutoCapitalizationType");
        setAutoCorrectionType = textFieldJavaClass->GetMethod<void, jint>("setAutoCorrectionType");
        setSpellCheckingType = textFieldJavaClass->GetMethod<void, jint>("setSpellCheckingType");
        setKeyboardAppearanceType = textFieldJavaClass->GetMethod<void, jint>("setKeyboardAppearanceType");
        setKeyboardType = textFieldJavaClass->GetMethod<void, jint>("setKeyboardType");
        setReturnKeyType = textFieldJavaClass->GetMethod<void, jint>("setReturnKeyType");
        setEnableReturnKeyAutomatically = textFieldJavaClass->GetMethod<void, jboolean>("setEnableReturnKeyAutomatically");
        getCursorPos = textFieldJavaClass->GetMethod<jint>("getCursorPos");
        setCursorPos = textFieldJavaClass->GetMethod<void, jint>("setCursorPos");
        update = textFieldJavaClass->GetMethod<void>("update");
    }
    catch (const JNI::Exception& e)
    {
        Logger::Error("[TextFieldControl] failed to init java bridge: %s", e.what());
        DVASSERT(false, e.what());
        return;
    }

    std::weak_ptr<TextFieldPlatformImpl>* selfWeakPtr = new std::weak_ptr<TextFieldPlatformImpl>(shared_from_this());
    jobject obj = PlatformApi::Android::CreateNativeControl(window, "com.dava.engine.DavaTextField", selfWeakPtr);
    if (obj != nullptr)
    {
        JNIEnv* env = JNI::GetEnv();
        javaTextField = env->NewGlobalRef(obj);
        env->DeleteLocalRef(obj);
    }
    else
    {
        delete selfWeakPtr;
        Logger::Error("[TextFieldControl] failed to create java textfield");
    }
}

void TextFieldPlatformImpl::OwnerIsDying()
{
    uiTextField = nullptr;
    uiTextFieldDelegate = nullptr;
    if (javaTextField != nullptr)
    {
        release(javaTextField);
        JNI::GetEnv()->DeleteGlobalRef(javaTextField);
        javaTextField = nullptr;
    }
}

void TextFieldPlatformImpl::SetVisible(bool visible)
{
    if (javaTextField != nullptr)
    {
        setVisible(javaTextField, visible ? JNI_TRUE : JNI_FALSE);
    }
}

void TextFieldPlatformImpl::SetIsPassword(bool password)
{
    if (javaTextField != nullptr)
    {
        setIsPassword(javaTextField, password ? JNI_TRUE : JNI_FALSE);
    }
}

void TextFieldPlatformImpl::SetMaxLength(int32 value)
{
    if (javaTextField != nullptr)
    {
        maxTextLength = value;
        setMaxLength(javaTextField, value);
    }
}

void TextFieldPlatformImpl::OpenKeyboard()
{
    if (javaTextField != nullptr)
    {
        openKeyboard(javaTextField);
    }
}

void TextFieldPlatformImpl::CloseKeyboard()
{
    if (javaTextField != nullptr)
    {
        closeKeyboard(javaTextField);
    }
}

void TextFieldPlatformImpl::UpdateRect(const Rect& rect)
{
    if (javaTextField != nullptr)
    {
        if (controlRect != rect)
        {
            controlRect = rect;

            Rect rc = GetEngineContext()->uiControlSystem->vcs->ConvertVirtualToInput(rect);
            rc.dx = std::max(0.0f, rc.dx);
            rc.dy = std::max(0.0f, rc.dy);
            setRect(javaTextField, rc.x, rc.y, rc.dx, rc.dy);
        }
        update(javaTextField);
    }
}

void TextFieldPlatformImpl::SetText(const WideString& text)
{
    if (javaTextField != nullptr)
    {
        if (text != curText)
        {
            curText = text;
            if (maxTextLength >= 0)
            {
                size_t n = std::min(curText.length(), static_cast<size_t>(maxTextLength));
                curText.resize(n);
            }

            JNIEnv* env = JNI::GetEnv();
            jstring jstr = JNI::WideStringToJavaString(curText, env);
            setText(javaTextField, jstr);
            env->DeleteLocalRef(jstr);

            if (curText.empty())
            { // Immediately remove sprite image if new text is empty to get rid of some flickering
                uiTextField->RemoveComponent<UIControlBackground>();
            }
        }
    }
}

void TextFieldPlatformImpl::GetText(WideString& text) const
{
    text = curText;
}

void TextFieldPlatformImpl::SetTextColor(const Color& color)
{
    if (javaTextField != nullptr)
    {
        jint a = static_cast<jint>(color.a * 255.f);
        jint r = static_cast<jint>(color.r * 255.f);
        jint g = static_cast<jint>(color.g * 255.f);
        jint b = static_cast<jint>(color.b * 255.f);
        setTextColor(javaTextField, a, r, g, b);
    }
}

void TextFieldPlatformImpl::SetTextAlign(int32 align)
{
    if (javaTextField != nullptr)
    {
        textAlign = align;
        setTextAlign(javaTextField, align);
    }
}

void TextFieldPlatformImpl::SetTextUseRtlAlign(bool useRtlAlign)
{
    if (javaTextField != nullptr)
    {
        textRtlAlign = useRtlAlign;
        setTextUseRtlAlign(javaTextField, useRtlAlign);
    }
}

void TextFieldPlatformImpl::SetFontSize(float32 virtualFontSize)
{
    if (javaTextField != nullptr && virtualFontSize > 0.f) // Font size must be greater than 0
    {
        // TODO: window.GetVirtualCoordinatesSystem
        float32 fontSize = GetEngineContext()->uiControlSystem->vcs->ConvertVirtualToInputY(virtualFontSize);
        setFontSize(javaTextField, fontSize);
    }
}

void TextFieldPlatformImpl::SetDelegate(UITextFieldDelegate* textFieldDelegate)
{
    uiTextFieldDelegate = textFieldDelegate;
}

void TextFieldPlatformImpl::SetMultiline(bool enable)
{
    if (javaTextField != nullptr)
    {
        multiline = enable;
        setMultiline(javaTextField, enable);
    }
}

void TextFieldPlatformImpl::SetInputEnabled(bool enable)
{
    if (javaTextField != nullptr)
    {
        setInputEnabled(javaTextField, enable);
    }
}

void TextFieldPlatformImpl::SetAutoCapitalizationType(int32 value)
{
    if (javaTextField != nullptr)
    {
        setAutoCapitalizationType(javaTextField, value);
    }
}

void TextFieldPlatformImpl::SetAutoCorrectionType(int32 value)
{
    if (javaTextField != nullptr)
    {
        setAutoCorrectionType(javaTextField, value);
    }
}

void TextFieldPlatformImpl::SetSpellCheckingType(int32 value)
{
    if (javaTextField != nullptr)
    {
        setSpellCheckingType(javaTextField, value);
    }
}

void TextFieldPlatformImpl::SetKeyboardAppearanceType(int32 value)
{
    if (javaTextField != nullptr)
    {
        setKeyboardAppearanceType(javaTextField, value);
    }
}

void TextFieldPlatformImpl::SetKeyboardType(int32 value)
{
    if (javaTextField != nullptr)
    {
        setKeyboardType(javaTextField, value);
    }
}

void TextFieldPlatformImpl::SetReturnKeyType(int32 value)
{
    if (javaTextField != nullptr)
    {
        setReturnKeyType(javaTextField, value);
    }
}

void TextFieldPlatformImpl::SetEnableReturnKeyAutomatically(bool value)
{
    if (javaTextField != nullptr)
    {
        setEnableReturnKeyAutomatically(javaTextField, value ? JNI_TRUE : JNI_FALSE);
    }
}

uint32 TextFieldPlatformImpl::GetCursorPos() const
{
    if (javaTextField != nullptr)
    {
        return static_cast<uint32>(getCursorPos(javaTextField));
    }
    return 0;
}

void TextFieldPlatformImpl::SetCursorPos(uint32 pos)
{
    if (javaTextField != nullptr)
    {
        setCursorPos(javaTextField, static_cast<jint>(pos));
    }
}

void TextFieldPlatformImpl::nativeOnFocusChange(JNIEnv* env, jboolean hasFocus)
{
    RunOnMainThreadAsync([this, hasFocus]() {
        OnFocusChanged(hasFocus == JNI_TRUE);
    });
}

void TextFieldPlatformImpl::nativeOnKeyboardShown(JNIEnv* env, jint x, jint y, jint w, jint h)
{
    Rect keyboardRect(static_cast<float32>(x),
                      static_cast<float32>(y),
                      static_cast<float32>(w),
                      static_cast<float32>(h));

    RunOnMainThreadAsync([this, keyboardRect]() {
        const Rect keyboardVirtualRect = window->GetUIControlSystem()->vcs->ConvertInputToVirtual(keyboardRect);
        OnKeyboardShown(keyboardVirtualRect);
    });
}

void TextFieldPlatformImpl::nativeOnKeyboardHidden(JNIEnv* env)
{
    RunOnMainThreadAsync([this]() {
        if (uiTextField != nullptr)
        {
            uiTextField->OnKeyboardHidden();
        }
    });
}

void TextFieldPlatformImpl::nativeOnEnterPressed(JNIEnv* env)
{
    RunOnMainThreadAsync([this]() {
        OnEnterPressed();
    });
}

jboolean TextFieldPlatformImpl::nativeOnKeyPressed(JNIEnv* env, jint replacementStart, jint replacementLength, jstring replaceWith)
{
    bool accept = true;
    WideString s = JNI::JavaStringToWideString(replaceWith, env);
    RunOnMainThread([this, replacementStart, replacementLength, s, &accept]() mutable {
        accept = OnKeyPressed(replacementStart, replacementLength, s);
    });
    return accept ? JNI_TRUE : JNI_FALSE;
}

void TextFieldPlatformImpl::nativeOnTextChanged(JNIEnv* env, jstring newText, jboolean programmaticTextChange)
{
    WideString s = JNI::JavaStringToWideString(newText, env);
    RunOnMainThreadAsync([this, s, programmaticTextChange]() {
        OnTextChanged(s, programmaticTextChange == JNI_TRUE);
    });
}

void TextFieldPlatformImpl::nativeOnTextureReady(JNIEnv* env, jintArray pixels, jint w, jint h)
{
    RefPtr<Image> image;
    if (pixels != nullptr)
    {
        jint* arrayElements = env->GetIntArrayElements(pixels, nullptr);

        uint8* imageBytes = reinterpret_cast<uint8*>(arrayElements);
        image.Set(Image::CreateFromData(w, h, FORMAT_RGBA8888, imageBytes));
        ImageConvert::SwapRedBlueChannels(image.Get());

        // JNI_ABORT tells to free the buffer without copying back the possible changes
        env->ReleaseIntArrayElements(pixels, arrayElements, JNI_ABORT);
    }

    RunOnMainThreadAsync([this, image]() {
        if (uiTextField != nullptr)
        {
            // We cannot create Sprite from texture if renderer is suspended (since it requires to execute OpenGL commands)
            // So if this is the case, wait until app is resumed and proceed

            if (!Engine::Instance()->IsSuspended())
            {
                SetSpriteFromImage(image.Get());
            }
            else
            {
                std::shared_ptr<Token> connectionTokenPtr = std::make_shared<Token>();
                *connectionTokenPtr = Engine::Instance()->resumed.Connect([this, image, connectionTokenPtr]() {
                    Engine::Instance()->resumed.Disconnect(*connectionTokenPtr);
                    SetSpriteFromImage(image.Get());
                });
            }
        }
    });
}

void TextFieldPlatformImpl::SetSpriteFromImage(Image* image) const
{
    if (uiTextField != nullptr)
    {
        RefPtr<Sprite> sprite;

        if (image != nullptr)
        {
            const Rect textFieldRect = uiTextField->GetRect();
            RefPtr<Texture> texture(Texture::CreateFromData(FORMAT_RGBA8888, image->GetData(), image->GetWidth(), image->GetHeight(), false));
            sprite.Set(Sprite::CreateFromTexture(texture.Get(), 0, 0, texture->GetWidth(), texture->GetHeight(), textFieldRect.dx, textFieldRect.dy));
        }

        UIControlBackground* bg = uiTextField->GetOrCreateComponent<UIControlBackground>();
        bg->SetSprite(sprite.Get(), 0);
    }
}

void TextFieldPlatformImpl::OnFocusChanged(bool hasFocus)
{
    if (uiTextField != nullptr)
    {
        if (hasFocus)
        {
            UIControl* curFocused = GetEngineContext()->uiControlSystem->GetFocusedControl();
            if (curFocused != uiTextField && FocusHelpers::CanFocusControl(uiTextField))
            {
                uiTextField->SetFocused();
            }
            uiTextField->StartEdit();
            uiTextField->RemoveComponent<UIControlBackground>();
        }
        else
        {
            uiTextField->StopEdit();
        }
    }
}

void TextFieldPlatformImpl::OnKeyboardShown(const Rect& keyboardRect)
{
    if (nullptr != uiTextField)
    {
        uiTextField->OnKeyboardShown(keyboardRect);
    }
}

void TextFieldPlatformImpl::OnEnterPressed()
{
    if (uiTextFieldDelegate != nullptr)
    {
        uiTextFieldDelegate->TextFieldShouldReturn(uiTextField);
    }
}

bool TextFieldPlatformImpl::OnKeyPressed(int32 replacementStart, int32 replacementLength, WideString& replaceWith)
{
    if (uiTextFieldDelegate != nullptr)
    {
        return uiTextFieldDelegate->TextFieldKeyPressed(uiTextField, replacementStart, replacementLength, replaceWith);
    }
    return true;
}

void TextFieldPlatformImpl::OnTextChanged(const WideString& newText, bool programmaticTextChange)
{
    if (newText != curText)
    {
        if (uiTextFieldDelegate != nullptr)
        {
            UITextFieldDelegate::eReason reason = programmaticTextChange ? UITextFieldDelegate::eReason::CODE : UITextFieldDelegate::eReason::USER;
            uiTextFieldDelegate->TextFieldOnTextChanged(uiTextField, newText, curText, reason);
        }
        curText = newText;
    }
}

} // namespace DAVA 

#endif // __DAVAENGINE_ANDROID__

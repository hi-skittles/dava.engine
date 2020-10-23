#include "GPUTest.h"
#include <UI/Render/UIDebugRenderComponent.h>

GPUTest::GPUTest(TestBed& app)
    : BaseScreen(app, "GPUTest")
{
}

void GPUTest::LoadResources()
{
    BaseScreen::LoadResources();

    ScopedPtr<Font> font(FTFont::Create("~res:/TestBed/Fonts/korinna.ttf"));

    { //create control to display texture, loaded with default GPU
        ScopedPtr<UIControl> textureBackground(new UIControl(Rect(10.f, 10.f, 256.f, 128.f)));
        textureBackground->GetOrCreateComponent<UIDebugRenderComponent>();
        ScopedPtr<Texture> texture(Texture::CreateFromFile("~res:/TestBed/TestData/GPUTest/Texture/texture.tex"));
        ScopedPtr<Sprite> sprite(Sprite::CreateFromTexture(texture, 0, 0, 128.f, 64.f));
        UIControlBackground* textureBackgroundBg = textureBackground->GetOrCreateComponent<UIControlBackground>();
        textureBackgroundBg->SetSprite(sprite, 0);
        AddControl(textureBackground);
    }

    { //create control to display sprite, loaded with default GPU
        ScopedPtr<UIControl> spriteBackground(new UIControl(Rect(276.f, 10.f, 256.f, 128.f)));
        spriteBackground->GetOrCreateComponent<UIDebugRenderComponent>();
        ScopedPtr<Sprite> sprite(Sprite::Create("~res:/TestBed/TestData/GPUTest/Sprite/texture"));
        UIControlBackground* spriteBackgroundBg = spriteBackground->GetOrCreateComponent<UIControlBackground>();
        spriteBackgroundBg->SetSprite(sprite, 0);
        AddControl(spriteBackground);
    }

    { //create control to display text with name of GPU, detected by system
        ScopedPtr<UIStaticText> textControl(new UIStaticText(Rect(10.0f, 148.0f, 512.f, 30.f)));
        textControl->GetOrCreateComponent<UIDebugRenderComponent>();
        textControl->SetTextColor(Color::White);
        textControl->SetFont(font);
        textControl->SetFontSize(16.f);
        textControl->SetTextAlign(ALIGN_HCENTER | ALIGN_VCENTER);

        String text = Format("Detected GPU: %s", GlobalEnumMap<eGPUFamily>::Instance()->ToString(static_cast<eGPUFamily>(DeviceInfo::GetGPUFamily())));
        textControl->SetText(UTF8Utils::EncodeToWideString(text));
        AddControl(textControl);
    }
}

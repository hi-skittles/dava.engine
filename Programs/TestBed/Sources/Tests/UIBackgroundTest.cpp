#include "Tests/UIBackgroundTest.h"
#include "UI/UIControlBackground.h"

#include "Render/2D/Sprite.h"

using namespace DAVA;

UIBackgroundTest::UIBackgroundTest(TestBed& app)
    : BaseScreen(app, "UIBackgroundTest")
{
}

void UIBackgroundTest::LoadResources()
{
    BaseScreen::LoadResources();
    //TODO: Initialize resources here

    Font* font = FTFont::Create("~res:/TestBed/Fonts/korinna.ttf");
    DVASSERT(font);

    ScopedPtr<Sprite> sprite(Sprite::CreateFromSourceFile("~res:/TestBed/TestData/UI/box.png"));

    text_orig = new UIStaticText(Rect(0, 0, 100, 100));
    text_orig->SetText(L"orig");
    text_orig->SetFont(font);
    text_orig->SetFontSize(14.f);
    UIControlBackground* text_origBg = text_orig->GetOrCreateComponent<UIControlBackground>();
    text_origBg->SetDrawType(UIControlBackground::DRAW_STRETCH_BOTH);
    text_origBg->SetSprite(sprite, 0);

    text_modif_h = new UIStaticText(Rect(0, 120, 100, 100));
    text_modif_h->SetText(L"H");
    text_modif_h->SetFont(font);
    text_modif_h->SetFontSize(14.f);
    UIControlBackground* text_modif_hBg = text_modif_h->GetOrCreateComponent<UIControlBackground>();
    text_modif_hBg->SetDrawType(UIControlBackground::DRAW_STRETCH_BOTH);
    text_modif_hBg->SetModification(ESM_HFLIP);
    text_modif_hBg->SetSprite(sprite, 0);

    text_modif_v = new UIStaticText(Rect(120, 0, 100, 100));
    text_modif_v->SetText(L"V");
    text_modif_v->SetFont(font);
    text_modif_v->SetFontSize(14.f);
    UIControlBackground* text_modif_vBg = text_modif_v->GetOrCreateComponent<UIControlBackground>();
    text_modif_vBg->SetDrawType(UIControlBackground::DRAW_STRETCH_BOTH);
    text_modif_vBg->SetModification(ESM_VFLIP);
    text_modif_vBg->SetSprite(sprite, 0);

    text_modif_hv = new UIStaticText(Rect(120, 120, 100, 100));
    text_modif_hv->SetText(L"HV");
    text_modif_hv->SetFont(font);
    text_modif_hv->SetFontSize(14.f);
    UIControlBackground* text_modif_hvBg = text_modif_hv->GetOrCreateComponent<UIControlBackground>();
    text_modif_hvBg->SetDrawType(UIControlBackground::DRAW_STRETCH_BOTH);
    text_modif_hvBg->SetModification(ESM_VFLIP | ESM_HFLIP);
    text_modif_hvBg->SetSprite(sprite, 0);

    AddControl(text_orig);
    AddControl(text_modif_h);
    AddControl(text_modif_v);
    AddControl(text_modif_hv);
}

void UIBackgroundTest::UnloadResources()
{
    BaseScreen::UnloadResources();
    //TODO: Release resources here

    SafeRelease(text_orig);
    SafeRelease(text_modif_h);
    SafeRelease(text_modif_v);
    SafeRelease(text_modif_hv);
}

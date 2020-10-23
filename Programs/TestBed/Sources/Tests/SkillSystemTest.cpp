#include "SkillSystemTest.h"
#include <UI/Render/UIDebugRenderComponent.h>
#include "Input/Keyboard.h"
#include "Engine/Engine.h"
#include "DeviceManager/DeviceManager.h"

#include <iomanip>

SkillBtn::SkillBtn(UIControl* parent, Rect rect, Color color, const WideString& name, const Skill& skill)
    : UIButton(rect)
    , parent(parent)
    , color(color)
    , skill(skill)
{
    DVASSERT(parent != nullptr);

    skillCdColorStatus = new UIButton();
    skillCdColorStatus->GetOrCreateComponent<UIControlBackground>()->SetColor(Color::Red);
    skillCdColorStatus->GetOrCreateComponent<UIControlBackground>()->SetDrawType(UIControlBackground::eDrawType::DRAW_FILL);
    skillCdColorStatus->SetVisibilityFlag(false);
    AddControl(skillCdColorStatus);

    ResetColorStatus();

    Font* font = FTFont::Create("~res:/TestBed/Fonts/korinna.ttf");
    DVASSERT(font);

    skillName = new UIStaticText(Rect({ 0.f, 0.f }, GetSize()));
    skillName->SetMultiline(true);
    skillName->SetFont(font);
    skillName->SetFontSize(25.f);
    skillName->SetTextColor(Color::White);
    skillName->SetText(name);
    AddControl(skillName);

    skillCdTextStatus = new UIStaticText(Rect({ 0.f, 0.f }, GetSize()));
    skillCdTextStatus->SetFont(font);
    skillCdTextStatus->SetFontSize(25.f);
    skillCdTextStatus->SetTextColor(Color::White);
    skillCdTextStatus->SetVisibilityFlag(false);
    skillCdTextStatus->SetTextAlign(ALIGN_HCENTER | ALIGN_BOTTOM);

    AddControl(skillCdTextStatus);

    parent->AddControl(this);

    SafeRelease(font);

    Engine::Instance()->update.Connect(this, &SkillBtn::OnUpdate);
}

SkillBtn::~SkillBtn()
{
    Engine::Instance()->update.Disconnect(this);

    parent->RemoveControl(this);

    SafeRelease(skillName);
    SafeRelease(skillCdTextStatus);
    SafeRelease(skillCdColorStatus);
}

void SkillBtn::OnUpdate(float32)
{
    if (!skill.enabled)
    {
        GetOrCreateComponent<UIDebugRenderComponent>()->SetDrawColor(Color::Red);
        skillCdTextStatus->SetVisibilityFlag(false);
        ResetColorStatus();
        return;
    }

    if (skill.currentCd != 0.f)
    {
        GetOrCreateComponent<UIDebugRenderComponent>()->SetDrawColor(Color::Red);

        skillCdTextStatus->SetVisibilityFlag(true);

        std::wstringstream wst;
        wst << std::fixed << std::setprecision(2) << skill.currentCd;
        skillCdTextStatus->SetText(wst.str());

        FillColorStatus(skill.cd / skill.currentCd);
    }
    else
    {
        GetOrCreateComponent<UIDebugRenderComponent>()->SetDrawColor(color);

        skillCdTextStatus->SetVisibilityFlag(false);

        ResetColorStatus();
    }
}

void SkillBtn::ResetColorStatus()
{
    skillCdColorStatus->SetPosition({ 0.f, 0.f });
    skillCdColorStatus->SetSize(GetSize());
    skillCdColorStatus->SetVisibilityFlag(false);
}

void SkillBtn::FillColorStatus(float32 p)
{
    Vector2 size = GetSize();
    skillCdColorStatus->SetVisibilityFlag(true);
    Vector2 pos = skillCdColorStatus->GetPosition();
    float32 dy = size.dy * (1.f / p);
    skillCdColorStatus->SetSize({ size.dx, dy });
    skillCdColorStatus->SetPosition({ pos.x, size.dy - dy });
}

SkillSystemTest::SkillSystemTest(TestBed& app)
    : BaseScreen(app, "SkillSystemTest")
{
}

void SkillSystemTest::OnUpdate(float32 timeElapsed)
{
    static ActionSystem* actionSystem = GetEngineContext()->actionSystem;

    if (set.digitalBindings.empty())
    {
        return;
    }

    for (auto& p : actionToSkill)
    {
        if (actionSystem->GetDigitalActionState(p.first))
        {
            OnAction(p.first);
        }
    }
}

void SkillSystemTest::UpdateCd(float32 timeElapsed)
{
    for (auto& p : actionToSkill)
    {
        Skill* skill = p.second;

        if (skill->currentCd != 0.f)
        {
            if (skill->currentCd - timeElapsed > 0.f)
            {
                skill->currentCd -= timeElapsed;
            }
            else
            {
                skill->currentCd = 0.f;
            }
        }
    }
}

void SkillSystemTest::OnAction(FastName actionId)
{
    Skill* skill = actionToSkill[actionId];

    if (skill->enabled && skill->currentCd == 0.f)
    {
        skill->currentCd = skill->cd;
    }
}

void SkillSystemTest::OnButtonReleased(BaseObject* obj, void* data, void* callerData)
{
    SkillBtn* btn = static_cast<SkillBtn*>(obj);

    FastName actionId = btnToAction[btn];

    OnAction(actionId);
}

void SkillSystemTest::OnSkillDisable(BaseObject* obj, void* data, void* callerData)
{
    SkillBtn* btn = static_cast<SkillBtn*>(obj);
    Skill* skill = static_cast<Skill*>(data);

    skill->enabled = !skill->enabled;
    btn->SetStateText(0xFF, skill->enabled ? L"Disable" : L"Enable");
}

void SkillSystemTest::LoadResources()
{
    BaseScreen::LoadResources();

    fireball = new Skill(5.f);
    frostBite = new Skill(6.f);
    lightningStrike = new Skill(7.f);

    Font* font = FTFont::Create("~res:/TestBed/Fonts/korinna.ttf");
    DVASSERT(font);

    FastName fireballAction("Fireball");
    DigitalBinding fireballBind;
    fireballBind.actionId = fireballAction;
    fireballBind.digitalElements[0] = eInputElements::KB_Q;
    fireballBind.digitalStates[0] = DigitalElementState::JustPressed();

    fireballBtn = new SkillBtn(this, { 31, 250, 300, 300 }, Color::Yellow, L"Fireball (Q)", *fireball);
    btnToAction[fireballBtn] = fireballAction;
    actionToSkill[fireballAction] = fireball;

    FastName frostBiteAction("FrostBite");
    DigitalBinding frostBiteBind;
    frostBiteBind.actionId = frostBiteAction;
    frostBiteBind.digitalElements[0] = eInputElements::KB_W;
    frostBiteBind.digitalStates[0] = DigitalElementState::JustPressed();

    frostBiteBtn = new SkillBtn(this, { 362, 250, 300, 300 }, Color::White, L"Frostbite (W)", *frostBite);
    btnToAction[frostBiteBtn] = frostBiteAction;
    actionToSkill[frostBiteAction] = frostBite;

    FastName lightningStrikeAction("LightningStrike");
    DigitalBinding lightningStrikeBind;
    lightningStrikeBind.actionId = lightningStrikeAction;
    lightningStrikeBind.digitalElements[0] = eInputElements::KB_E;
    lightningStrikeBind.digitalStates[0] = DigitalElementState::JustPressed();

    lightningStrikeBtn = new SkillBtn(this, { 693, 250, 300, 300 }, Color::Cyan, L"Lightning Strike (E)", *lightningStrike);
    btnToAction[lightningStrikeBtn] = lightningStrikeAction;
    actionToSkill[lightningStrikeAction] = lightningStrike;

    for (auto& p : btnToAction)
    {
        p.first->AddEvent(UIControl::EVENT_TOUCH_UP_INSIDE, Message(this, &SkillSystemTest::OnButtonReleased));
        p.first->AddEvent(UIControl::EVENT_TOUCH_UP_OUTSIDE, Message(this, &SkillSystemTest::OnButtonReleased));
    }

    const Keyboard* kb = GetEngineContext()->deviceManager->GetKeyboard();

    if (kb != nullptr)
    {
        set.digitalBindings.push_back(fireballBind);
        set.digitalBindings.push_back(frostBiteBind);
        set.digitalBindings.push_back(lightningStrikeBind);

        GetEngineContext()->actionSystem->BindSet(set, kb->GetId());
    }

    disableFireball = new UIButton({ 31, 570, 300, 40 });
    disableFireball->SetStateFont(0xFF, font);
    disableFireball->SetStateFontSize(0xFF, 25.f);
    disableFireball->SetStateColor(0xFF, Color::White);
    disableFireball->SetStateText(0xFF, fireball->enabled ? L"Disable" : L"Enable");
    disableFireball->GetOrCreateComponent<UIDebugRenderComponent>();
    disableFireball->AddEvent(UIControl::EVENT_TOUCH_UP_INSIDE, Message(this, &SkillSystemTest::OnSkillDisable, fireball));
    AddControl(disableFireball);

    disableFrostBite = new UIButton({ 362, 570, 300, 40 });
    disableFrostBite->SetStateFont(0xFF, font);
    disableFrostBite->SetStateFontSize(0xFF, 25.f);
    disableFrostBite->SetStateColor(0xFF, Color::White);
    disableFrostBite->SetStateText(0xFF, frostBite->enabled ? L"Disable" : L"Enable");
    disableFrostBite->GetOrCreateComponent<UIDebugRenderComponent>();
    disableFrostBite->AddEvent(UIControl::EVENT_TOUCH_UP_INSIDE, Message(this, &SkillSystemTest::OnSkillDisable, frostBite));
    AddControl(disableFrostBite);

    disableLightningStrike = new UIButton({ 693, 570, 300, 40 });
    disableLightningStrike->SetStateFont(0xFF, font);
    disableLightningStrike->SetStateFontSize(0xFF, 25.f);
    disableLightningStrike->SetStateColor(0xFF, Color::White);
    disableLightningStrike->SetStateText(0xFF, lightningStrike->enabled ? L"Disable" : L"Enable");
    disableLightningStrike->GetOrCreateComponent<UIDebugRenderComponent>();
    disableLightningStrike->AddEvent(UIControl::EVENT_TOUCH_UP_INSIDE, Message(this, &SkillSystemTest::OnSkillDisable, lightningStrike));
    AddControl(disableLightningStrike);

    SafeRelease(font);

    Engine::Instance()->update.Connect(this, &SkillSystemTest::OnUpdate);
    Engine::Instance()->update.Connect(this, &SkillSystemTest::UpdateCd);
}

void SkillSystemTest::UnloadResources()
{
    BaseScreen::UnloadResources();

    Engine::Instance()->update.Disconnect(this);

    GetEngineContext()->actionSystem->UnbindAllSets();

    set.digitalBindings.clear();
    set.analogBindings.clear();

    btnToAction.clear();
    actionToSkill.clear();

    SafeRelease(fireballBtn);
    SafeRelease(frostBiteBtn);
    SafeRelease(lightningStrikeBtn);

    SafeRelease(disableFireball);
    SafeRelease(disableFrostBite);
    SafeRelease(disableLightningStrike);

    delete fireball;
    delete frostBite;
    delete lightningStrike;
}

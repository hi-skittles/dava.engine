#pragma once

#include "DAVAEngine.h"
#include "Infrastructure/BaseScreen.h"
#include "Input/ActionSystem.h"
#include "Base/FastName.h"

using namespace DAVA;

struct Skill
{
    bool enabled;
    float32 currentCd;
    const float32 cd;

    Skill(float32 cd)
        : enabled(true)
        , cd(cd)
        , currentCd(0.f)
    {
    }
};

class SkillBtn : public UIButton
{
public:
    SkillBtn(UIControl* parent, Rect rect, Color color, const WideString& name, const Skill& skill);
    ~SkillBtn();

private:
    void OnUpdate(float32 t);
    void ResetColorStatus();
    void FillColorStatus(float32 p);

private:
    UIStaticText* skillName;
    UIStaticText* skillCdTextStatus;
    UIButton* skillCdColorStatus;

    UIControl* parent;
    Color color;
    const Skill& skill;
};

class SkillSystemTest : public BaseScreen
{
public:
    SkillSystemTest(TestBed& app);
    ~SkillSystemTest() = default;

protected:
    void LoadResources() override;
    void UnloadResources() override;

private:
    void OnUpdate(float32 timeElapsed);
    void UpdateCd(float32 timeElapsed);
    void OnAction(FastName actionId);
    void OnButtonReleased(BaseObject* obj, void* data, void* callerData);
    void OnSkillDisable(BaseObject* obj, void* data, void* callerData);

private:
    Skill* fireball;
    Skill* frostBite;
    Skill* lightningStrike;

    SkillBtn* fireballBtn;
    SkillBtn* frostBiteBtn;
    SkillBtn* lightningStrikeBtn;

    UIButton* disableFireball;
    UIButton* disableFrostBite;
    UIButton* disableLightningStrike;

    ActionSet set;

    UnorderedMap<SkillBtn*, FastName> btnToAction;
    UnorderedMap<FastName, Skill*> actionToSkill;
};
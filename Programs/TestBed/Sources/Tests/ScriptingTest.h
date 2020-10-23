#pragma once

#include "Infrastructure/BaseScreen.h"
#include "Functional/Function.h"
#include "Base/Any.h"

class ScriptingTest : public BaseScreen
{
public:
    ScriptingTest(TestBed& app);

protected:
    void LoadResources() override;
    void UnloadResources() override;
    void Update(DAVA::float32 timeElapsed) override;

private:
    void CreateScript();
    void Run(DAVA::Function<DAVA::int32()> func);
    DAVA::String AnyToString(const DAVA::Any& any);

    DAVA::RefPtr<DAVA::UITextField> scriptText;
    DAVA::RefPtr<DAVA::UITextField> intArgText;
    DAVA::RefPtr<DAVA::UITextField> strArgText;
    DAVA::RefPtr<DAVA::UIStaticText> outputText;
    DAVA::RefPtr<DAVA::UIStaticText> timeText;
};

#pragma once

#include "DAVAEngine.h"
#include "Infrastructure/BaseScreen.h"
#include <Base/RefPtr.h>
#include <UI/UIControl.h>
#include <UI/UITextField.h>

class TestBed;
struct TextTestCase;

using namespace DAVA;

class TextSystemTest : public BaseScreen
{
    enum eState
    {
        STOPPED,
        PLAYING
    };

public:
    TextSystemTest(TestBed& app);

protected:
    void LoadResources() override;
    void UnloadResources() override;
    void Update(float32 delta) override;

    void ChangeCurrentTest(int32 testIdx_);

private:
    eState state = STOPPED;
    UIStaticText* statusText;
    UIControl* holderControl;

    int32 testIdx;
    Vector<std::shared_ptr<TextTestCase>> objects;
    std::shared_ptr<TextTestCase> activeObject;
};

#ifndef __UIMOVIE_TEST_H__
#define __UIMOVIE_TEST_H__

#include "DAVAEngine.h"

#include "Infrastructure/BaseScreen.h"

using namespace DAVA;

class TestBed;
class UIMovieTest : public BaseScreen
{
protected:
    virtual ~UIMovieTest() = default;

public:
    UIMovieTest(TestBed& app);

    void LoadResources() override;
    void UnloadResources() override;

    void OnActive() override;
    void Update(float32 timeElapsed) override;

private:
    void UpdatePlayerStateText();
    UIButton* CreateUIButton(Font* font, const Rect& rect, const String& text,
                             void (UIMovieTest::*onClick)(BaseObject*, void*, void*));

    void ButtonPressed(BaseObject* obj, void* data, void* callerData);
    void ScaleButtonPressed(BaseObject* obj, void* data, void* callerData);

private:
    UIMovieView* movieView = nullptr;

    // Control buttons.
    UIButton* playButton = nullptr;
    UIButton* stopButton = nullptr;
    UIButton* pauseButton = nullptr;
    UIButton* resumeButton = nullptr;
    UIButton* hideButton = nullptr;
    UIButton* showButton = nullptr;

    UIButton* buttonScale0 = nullptr;
    UIButton* buttonScale1 = nullptr;
    UIButton* buttonScale2 = nullptr;
    UIButton* buttonScale3 = nullptr;

    UIStaticText* playerStateText = nullptr;
};

#endif // __UIMOVIE_TEST_H__

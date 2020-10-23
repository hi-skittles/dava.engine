#ifndef __TEXT_ALIGH_TEST_H__
#define __TEXT_ALIGH_TEST_H__

#include "Infrastructure/BaseScreen.h"

class TestBed;
class StaticTextTest : public BaseScreen
{
public:
    StaticTextTest(TestBed& app);

    void LoadResources() override;
    void UnloadResources() override;

    void SetPreviewText(const DAVA::WideString& text);
    void SetPreviewAlign(DAVA::int32 align);
    void SetPreviewFitting(DAVA::int32 fitting);
    void SetPreviewRequiredTextSize(bool enable);
    void SetPreviewMultiline(DAVA::int32 multilineType);

private:
    DAVA::UIButton* CreateButton(const DAVA::WideString& caption, const DAVA::Rect& rect, DAVA::int32 tag, DAVA::Font* font, const DAVA::Message& msg);

    void OnAlignButtonClick(BaseObject* sender, void* data, void* callerData);
    void OnFittingButtonClick(BaseObject* sender, void* data, void* callerData);
    void OnRequireTextSizeButtonClick(BaseObject* sender, void* data, void* callerData);
    void OnMultilineButtonClick(BaseObject* sender, void* data, void* callerData);
    void UpdateDebugLabel();

    DAVA::UIStaticText* previewText = nullptr;
    DAVA::UIStaticText* debugLabel = nullptr;
    DAVA::UITextField* inputText = nullptr;
    DAVA::UITextFieldDelegate* inputDelegate = nullptr;
    DAVA::UIButton* requireTextSizeButton = nullptr;
    DAVA::List<DAVA::UIButton*> alignButtons;
    DAVA::List<DAVA::UIButton*> fittingButtons;
    DAVA::List<DAVA::UIButton*> multilineButtons;

    bool needRequiredSize = false;
};

#endif //__MULTILINETEST_TEST_H__

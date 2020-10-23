#ifndef __FUNCTIONSIGNAL_TEST_H__
#define __FUNCTIONSIGNAL_TEST_H__

#include "Infrastructure/BaseScreen.h"

class TestBed;
class FunctionSignalTest : public BaseScreen
{
public:
    FunctionSignalTest(TestBed& app);

    void LoadResources() override;
    void UnloadResources() override;

    DAVA::UIButton* runButton;
    DAVA::UIStaticText* runResult;

protected:
    void OnButtonPress(DAVA::BaseObject* obj, void* data, void* callerData);
    void OnExitButton(DAVA::BaseObject* obj, void* data, void* callerData) override;
};

#endif //__FUNCTIONSIGNAL_TEST_H__

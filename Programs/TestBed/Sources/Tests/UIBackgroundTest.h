#ifndef __UIBACKGROUNDTEST_TEST_H__
#define __UIBACKGROUNDTEST_TEST_H__

#include "DAVAEngine.h"
#include "Infrastructure/BaseScreen.h"

using namespace DAVA;

class TestBed;
class UIBackgroundTest : public BaseScreen
{
public:
    UIBackgroundTest(TestBed& app);

protected:
    void LoadResources() override;
    void UnloadResources() override;

private:
    UIStaticText* text_orig;
    UIStaticText* text_modif_v;
    UIStaticText* text_modif_h;
    UIStaticText* text_modif_hv;
};

#endif //__UIBACKGROUNDTEST_TEST_H__

#ifndef __REPORT_SCREEN_H__
#define __REPORT_SCREEN_H__

#include "BaseScreen.h"
#include "Infrastructure/Utils/ControlHelpers.h"
#include "Tests/BaseTest.h"

class ReportScreen : public BaseScreen
{
public:
    void SetTestChain(const Vector<BaseTest*>& testsChain);
    bool IsFinished() const override;

protected:
    void LoadResources() override;

private:
    void CreateReportScreen();

    Vector<BaseTest*> testChain;
};

inline void ReportScreen::SetTestChain(const Vector<BaseTest*>& _testsChain)
{
    testChain = _testsChain;
}

#endif

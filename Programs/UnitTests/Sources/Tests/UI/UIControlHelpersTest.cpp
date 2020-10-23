#include "DAVAEngine.h"

#include "UI/UIControl.h"
#include "UI/UIScrollView.h"
#include "UI/UIControlHelpers.h"
#include "UnitTests/UnitTests.h"

using namespace DAVA;

DAVA_TESTCLASS (UIControlHelpersTest)
{
    UIScrollView* scrollView = nullptr;
    UIControl* smallControl = nullptr;
    UIControl* bigControl = nullptr;

    void Start(bool withPivots)
    {
        scrollView = new UIScrollView();
        scrollView->SetAutoUpdate(true);
        scrollView->SetSize(Vector2(100.0f, 100.0f));
        scrollView->GetContainer()->SetSize(Vector2(1000.0f, 1000.0f));
        if (withPivots)
        {
            scrollView->SetPivot(Vector2(0.5f, 0.5f));
            scrollView->SetPosition(Vector2(50.0f, 50.0f));
        }

        bigControl = new UIControl();
        bigControl->SetSize(Vector2(200.0f, 200.0f));
        bigControl->SetPosition(Vector2(200.0f, 200.0f));
        if (withPivots)
        {
            bigControl->SetPivot(Vector2(0.5f, 0.5f));
            bigControl->SetPosition(Vector2(300.0f, 300.0f));
        }
        scrollView->AddControlToContainer(bigControl);

        smallControl = new UIControl();
        smallControl->SetSize(Vector2(50.0f, 50.0f));
        smallControl->SetPosition(Vector2(0.0f, 0.0f));
        if (withPivots)
        {
            smallControl->SetPivot(Vector2(0.5f, 0.5f));
            smallControl->SetPosition(Vector2(25.0f, 25.0f));
        }
        bigControl->AddControl(smallControl);
    }

    void End()
    {
        SafeRelease(scrollView);
        SafeRelease(smallControl);
        SafeRelease(bigControl);
    }

    void SmallControlOnScreen_ScrollDontWork(bool withPivots)
    {
        Start(withPivots);

        Vector2 result;

        scrollView->SetScrollPosition(Vector2(-175.0f, -175.0f));
        result = smallControl->GetAbsolutePosition() - smallControl->GetPivotPoint();
        TEST_VERIFY(FLOAT_EQUAL(result.x, 25.0f));
        TEST_VERIFY(FLOAT_EQUAL(result.y, 25.0f));

        UIControlHelpers::ScrollToControl(smallControl);

        result = smallControl->GetAbsolutePosition() - smallControl->GetPivotPoint();
        TEST_VERIFY(FLOAT_EQUAL(result.x, 25.0f));
        TEST_VERIFY(FLOAT_EQUAL(result.y, 25.0f));

        End();
    }

    // UIControlHelpers::ScrollToControl
    DAVA_TEST (SmallControlOnScreen_ScrollDontWork)
    {
        SmallControlOnScreen_ScrollDontWork(false);
        SmallControlOnScreen_ScrollDontWork(true);
    }

    void SmallControlRightThanView_ScrollToLeft(bool withPivots)
    {
        Start(withPivots);

        Vector2 result;

        scrollView->SetScrollPosition(Vector2(0.0f, 0.0f));
        result = smallControl->GetAbsolutePosition() - smallControl->GetPivotPoint();
        TEST_VERIFY(FLOAT_EQUAL(result.x, 200.0f));
        TEST_VERIFY(FLOAT_EQUAL(result.y, 200.0f));

        UIControlHelpers::ScrollToControl(smallControl);

        result = smallControl->GetAbsolutePosition() - smallControl->GetPivotPoint();
        TEST_VERIFY(FLOAT_EQUAL(result.x, 50.0f));
        TEST_VERIFY(FLOAT_EQUAL(result.y, 50.0f));

        End();
    }

    // UIControlHelpers::ScrollToControl
    DAVA_TEST (SmallControlRightThanView_ScrollToLeft)
    {
        SmallControlRightThanView_ScrollToLeft(false);
        SmallControlRightThanView_ScrollToLeft(true);
    }

    void SmallControlLeftThanView_ScrollToRight(bool withPivots)
    {
        Start(withPivots);

        Vector2 result;

        // don't scroll if control visible
        scrollView->SetScrollPosition(Vector2(-500.0f, -500.0f));
        result = smallControl->GetAbsolutePosition() - smallControl->GetPivotPoint();
        TEST_VERIFY(FLOAT_EQUAL(result.x, -300.0f));
        TEST_VERIFY(FLOAT_EQUAL(result.y, -300.0f));

        UIControlHelpers::ScrollToControl(smallControl);

        result = smallControl->GetAbsolutePosition() - smallControl->GetPivotPoint();
        TEST_VERIFY(FLOAT_EQUAL(result.x, 0.0f));
        TEST_VERIFY(FLOAT_EQUAL(result.y, 0.0f));

        End();
    }

    // UIControlHelpers::ScrollToControl
    DAVA_TEST (SmallControlLeftThanView_ScrollToRight)
    {
        SmallControlLeftThanView_ScrollToRight(false);
        SmallControlLeftThanView_ScrollToRight(true);
    }

    void BigControlOnScreen_DontScroll(bool withPivots)
    {
        Start(withPivots);

        Vector2 result;

        scrollView->SetScrollPosition(Vector2(-250.0f, -250.0f));
        result = bigControl->GetAbsolutePosition() - bigControl->GetPivotPoint();
        TEST_VERIFY(FLOAT_EQUAL(result.x, -50.0f));
        TEST_VERIFY(FLOAT_EQUAL(result.y, -50.0f));

        UIControlHelpers::ScrollToControl(bigControl);

        result = bigControl->GetAbsolutePosition() - bigControl->GetPivotPoint();
        TEST_VERIFY(FLOAT_EQUAL(result.x, -50.0f));
        TEST_VERIFY(FLOAT_EQUAL(result.y, -50.0f));

        End();
    }

    // UIControlHelpers::ScrollToControl
    DAVA_TEST (BigControlOnScreen_DontScroll)
    {
        BigControlOnScreen_DontScroll(false);
        BigControlOnScreen_DontScroll(true);
    }

    void BigControlLeftThanView_ScrollToRight(bool withPivots)
    {
        Start(withPivots);

        Vector2 result;

        scrollView->SetScrollPosition(Vector2(-900.0f, -900));
        result = bigControl->GetAbsolutePosition() - bigControl->GetPivotPoint();
        TEST_VERIFY(FLOAT_EQUAL(result.x, -700.0f));
        TEST_VERIFY(FLOAT_EQUAL(result.y, -700.0f));

        UIControlHelpers::ScrollToControl(bigControl);

        result = bigControl->GetAbsolutePosition() - bigControl->GetPivotPoint();
        TEST_VERIFY(FLOAT_EQUAL(result.x, -100.0f));
        TEST_VERIFY(FLOAT_EQUAL(result.y, -100.0f));

        const bool toTopLeftForBigControls = true;
        UIControlHelpers::ScrollToControl(bigControl, toTopLeftForBigControls);

        result = bigControl->GetAbsolutePosition() - bigControl->GetPivotPoint();
        TEST_VERIFY(FLOAT_EQUAL(result.x, 0.0f));
        TEST_VERIFY(FLOAT_EQUAL(result.y, 0.0f));

        End();
    }

    // UIControlHelpers::ScrollToControl
    DAVA_TEST (BigControlLeftThanView_ScrollToRight)
    {
        BigControlLeftThanView_ScrollToRight(false);
        BigControlLeftThanView_ScrollToRight(true);
    }

    void BigControlRightThanView_ScrollToLeft(bool withPivots)
    {
        Start(withPivots);

        Vector2 result;

        scrollView->SetScrollPosition(Vector2(0.0f, 0.0f));
        result = bigControl->GetAbsolutePosition() - bigControl->GetPivotPoint();
        TEST_VERIFY(FLOAT_EQUAL(result.x, 200.0f));
        TEST_VERIFY(FLOAT_EQUAL(result.y, 200.0f));

        UIControlHelpers::ScrollToControl(bigControl);

        result = bigControl->GetAbsolutePosition() - bigControl->GetPivotPoint();
        TEST_VERIFY(FLOAT_EQUAL(result.x, 0.0f));
        TEST_VERIFY(FLOAT_EQUAL(result.y, 0.0f));

        const bool toTopLeftForBigControls = true;
        UIControlHelpers::ScrollToControl(bigControl, toTopLeftForBigControls);

        result = bigControl->GetAbsolutePosition() - bigControl->GetPivotPoint();
        TEST_VERIFY(FLOAT_EQUAL(result.x, 0.0f));
        TEST_VERIFY(FLOAT_EQUAL(result.y, 0.0f));

        End();
    }

    // UIControlHelpers::ScrollToControl
    DAVA_TEST (BigControlRightThanView_ScrollToLeft)
    {
        BigControlRightThanView_ScrollToLeft(false);
        BigControlRightThanView_ScrollToLeft(true);
    }
};

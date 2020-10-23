#include "DAVAEngine.h"
#include "UnitTests/UnitTests.h"

using namespace DAVA;

DAVA_TESTCLASS (UIControlHierarhyTest)
{
    static Vector<std::pair<FastName, FastName>> callSequence;
    class UITestControl;
    // screen
    // |-x
    // | |-1
    // | | |-1
    // | |
    // | |-2
    // | | |-1
    // | |
    // | |-3
    // | |
    // | |-z
    // | | |-1
    // | | |-2
    // | | |-3

    UIScreen* screen = nullptr;
    UITestControl* root = nullptr;
    UITestControl* x = nullptr;
    UITestControl* x1 = nullptr;
    UITestControl* x2 = nullptr;
    UITestControl* x3 = nullptr;
    UITestControl* x11 = nullptr;
    UITestControl* x21 = nullptr;

    UITestControl* z = nullptr;
    UITestControl* z1 = nullptr;
    UITestControl* z2 = nullptr;
    UITestControl* z3 = nullptr;

    UIScreen* MakeScreen(const char* name)
    {
        UIScreen* c = new UIScreen();
        c->SetName(name);
        return c;
    }

    UITestControl* MakeChild(UIControl * parent, const char* name)
    {
        UITestControl* c = new UITestControl();
        c->SetName(name);
        if (parent)
            parent->AddControl(c);
        return c;
    }

    void SetUp(const String& testName) override
    {
        screen = MakeScreen("screen");

        root = MakeChild(screen, "root");

        x = MakeChild(nullptr, "x");
        x1 = MakeChild(x, "x1");
        x2 = MakeChild(x, "x2");
        x3 = MakeChild(x, "x3");
        x11 = MakeChild(x1, "x11");
        x21 = MakeChild(x2, "x21");

        z = MakeChild(nullptr, "z");
        z1 = MakeChild(z, "z1");
        z2 = MakeChild(z, "z2");
        z3 = MakeChild(z, "z3");

        GetEngineContext()->uiControlSystem->SetScreen(screen);
        GetEngineContext()->uiControlSystem->Update();

        callSequence.clear();
    }

    void TearDown(const String& testName) override
    {
        GetEngineContext()->uiControlSystem->Reset();

        SafeRelease(screen);
        SafeRelease(x);
        SafeRelease(x1);
        SafeRelease(x2);
        SafeRelease(x3);
        SafeRelease(x11);
        SafeRelease(x21);

        SafeRelease(z);
        SafeRelease(z1);
        SafeRelease(z2);
        SafeRelease(z3);
    }

    // UIControl::AddControl
    // simple
    // in OnActive
    // in OnInactive
    // in OnVisible
    // in OnInvisible

    // UIControl::RemoveControl
    // RemoveControl:
    // simple
    // in OnActive
    // in OnInactive
    // in OnVisible
    // in OnInvisible

    // UIControl::AddControl
    DAVA_TEST (AddControlToInvisibleHierarhy_OnlyOnAppearCalls)
    {
        root->SetVisibilityFlag(false);
        callSequence.clear();
        root->AddControl(x);

        Vector<std::pair<FastName, FastName>> expectedCallSequence =
        {
          { FastName("OnActive"), FastName("x") },
          { FastName("OnActive"), FastName("x1") },
          { FastName("OnActive"), FastName("x11") },
          { FastName("OnActive"), FastName("x2") },
          { FastName("OnActive"), FastName("x21") },
          { FastName("OnActive"), FastName("x3") },
        };

        TEST_VERIFY(expectedCallSequence == callSequence);
    }

    // UIControl::AddControl
    DAVA_TEST (AddControlToNoneActiveHierarhy_NothingCalls)
    {
        root->RemoveFromParent();
        callSequence.clear();
        root->AddControl(x);

        TEST_VERIFY(callSequence.empty());
    }

    // UIControl::AddControl
    DAVA_TEST (AddControlToVisibleHierarhy_OnAppearAndOnBecomeVisibleCalls)
    {
        root->SetVisibilityFlag(true);
        callSequence.clear();
        root->AddControl(x);

        Vector<std::pair<FastName, FastName>> expectedCallSequence =
        {
          { FastName("OnActive"), FastName("x") },
          { FastName("OnActive"), FastName("x1") },
          { FastName("OnActive"), FastName("x11") },
          { FastName("OnActive"), FastName("x2") },
          { FastName("OnActive"), FastName("x21") },
          { FastName("OnActive"), FastName("x3") },

          { FastName("OnVisible"), FastName("x") },
          { FastName("OnVisible"), FastName("x1") },
          { FastName("OnVisible"), FastName("x11") },
          { FastName("OnVisible"), FastName("x2") },
          { FastName("OnVisible"), FastName("x21") },
          { FastName("OnVisible"), FastName("x3") },
        };

        TEST_VERIFY(expectedCallSequence == callSequence);
    }

    // UIControl::AddControl
    DAVA_TEST (AddControlInOnAppearCallbackToInvisibleHierarhy_OnAppearCalls)
    {
        root->SetVisibilityFlag(false);
        callSequence.clear();
        x1->onActiveCallback = [this]()
        {
            x->AddControl(z);
        };

        root->AddControl(x);

        Vector<std::pair<FastName, FastName>> expectedCallSequence =
        {
          { FastName("OnActive"), FastName("x") },
          { FastName("OnActive"), FastName("x1") },
          { FastName("OnActive"), FastName("z") },
          { FastName("OnActive"), FastName("z1") },
          { FastName("OnActive"), FastName("z2") },
          { FastName("OnActive"), FastName("z3") },
          { FastName("OnActive"), FastName("x11") },
          { FastName("OnActive"), FastName("x2") },
          { FastName("OnActive"), FastName("x21") },
          { FastName("OnActive"), FastName("x3") },
        };

        TEST_VERIFY(expectedCallSequence == callSequence);

        x1->onActiveCallback = nullptr;
    }

    // UIControl::AddControl
    DAVA_TEST (AddControlInOnDisappearCallbackToInvisibleHierarhy_OnDisappearCallsOnlyForXControlOriginalChildren)
    {
        root->SetVisibilityFlag(false);
        root->AddControl(x);
        callSequence.clear();
        x1->onInactiveCallback = [this]()
        {
            x->AddControl(z);
        };

        x->RemoveFromParent();

        Vector<std::pair<FastName, FastName>> expectedCallSequence =
        {
          { FastName("OnInactive"), FastName("x3") },
          { FastName("OnInactive"), FastName("x21") },
          { FastName("OnInactive"), FastName("x2") },
          { FastName("OnInactive"), FastName("x11") },
          { FastName("OnInactive"), FastName("x1") },
          { FastName("OnActive"), FastName("z") },
          { FastName("OnActive"), FastName("z1") },
          { FastName("OnActive"), FastName("z2") },
          { FastName("OnActive"), FastName("z3") },
          { FastName("OnInactive"), FastName("z3") },
          { FastName("OnInactive"), FastName("z2") },
          { FastName("OnInactive"), FastName("z1") },
          { FastName("OnInactive"), FastName("z") },
          { FastName("OnInactive"), FastName("x") },
        };

        TEST_VERIFY(expectedCallSequence == callSequence);

        x1->onInactiveCallback = nullptr;
    }

    // UIControl::AddControl
    DAVA_TEST (AddControlInOnBecomeVisibleCallbackToVisibleHierarhy_OnAppearAndBecomeVisibleCalls)
    {
        callSequence.clear();
        x1->onVisibleCallback = [this]()
        {
            x->AddControl(z);
        };

        root->AddControl(x);

        Vector<std::pair<FastName, FastName>> expectedCallSequence =
        {
          { FastName("OnActive"), FastName("x") },
          { FastName("OnActive"), FastName("x1") },
          { FastName("OnActive"), FastName("x11") },
          { FastName("OnActive"), FastName("x2") },
          { FastName("OnActive"), FastName("x21") },
          { FastName("OnActive"), FastName("x3") },

          { FastName("OnVisible"), FastName("x") },
          { FastName("OnVisible"), FastName("x1") },
          { FastName("OnActive"), FastName("z") },
          { FastName("OnActive"), FastName("z1") },
          { FastName("OnActive"), FastName("z2") },
          { FastName("OnActive"), FastName("z3") },
          { FastName("OnVisible"), FastName("z") },
          { FastName("OnVisible"), FastName("z1") },
          { FastName("OnVisible"), FastName("z2") },
          { FastName("OnVisible"), FastName("z3") },
          { FastName("OnVisible"), FastName("x11") },
          { FastName("OnVisible"), FastName("x2") },
          { FastName("OnVisible"), FastName("x21") },
          { FastName("OnVisible"), FastName("x3") },
        };

        TEST_VERIFY(expectedCallSequence == callSequence);

        x1->onVisibleCallback = nullptr;
    }

    // UIControl::AddControl
    DAVA_TEST (AddControlInOnBecomeInvisibleCallbackToVisibleHierarhy_OnDisappearAndBecomeInvisibleCalls)
    {
        root->AddControl(x);
        callSequence.clear();
        x1->onInvisibleCallback = [this]()
        {
            x->AddControl(z);
        };

        x->RemoveFromParent();

        Vector<std::pair<FastName, FastName>> expectedCallSequence =
        {
          { FastName("OnInvisible"), FastName("x3") },
          { FastName("OnInvisible"), FastName("x21") },
          { FastName("OnInvisible"), FastName("x2") },
          { FastName("OnInvisible"), FastName("x11") },
          { FastName("OnInvisible"), FastName("x1") },
          { FastName("OnActive"), FastName("z") },
          { FastName("OnActive"), FastName("z1") },
          { FastName("OnActive"), FastName("z2") },
          { FastName("OnActive"), FastName("z3") },
          { FastName("OnVisible"), FastName("z") },
          { FastName("OnVisible"), FastName("z1") },
          { FastName("OnVisible"), FastName("z2") },
          { FastName("OnVisible"), FastName("z3") },

          { FastName("OnInvisible"), FastName("z3") },
          { FastName("OnInvisible"), FastName("z2") },
          { FastName("OnInvisible"), FastName("z1") },
          { FastName("OnInvisible"), FastName("z") },

          { FastName("OnInvisible"), FastName("x") },
          { FastName("OnInactive"), FastName("z3") },
          { FastName("OnInactive"), FastName("z2") },
          { FastName("OnInactive"), FastName("z1") },
          { FastName("OnInactive"), FastName("z") },
          { FastName("OnInactive"), FastName("x3") },
          { FastName("OnInactive"), FastName("x21") },
          { FastName("OnInactive"), FastName("x2") },
          { FastName("OnInactive"), FastName("x11") },
          { FastName("OnInactive"), FastName("x1") },
          { FastName("OnInactive"), FastName("x") },
        };

        TEST_VERIFY(expectedCallSequence == callSequence);

        x1->onInvisibleCallback = nullptr;
    }

    // UIControl::RemoveControl
    DAVA_TEST (RemoveControlFromInvisibleHierarhy_OnlyOnDisappearCalls)
    {
        root->SetVisibilityFlag(false);
        root->AddControl(x);
        callSequence.clear();

        root->RemoveControl(x);

        Vector<std::pair<FastName, FastName>> expectedCallSequence =
        {
          { FastName("OnInactive"), FastName("x3") },
          { FastName("OnInactive"), FastName("x21") },
          { FastName("OnInactive"), FastName("x2") },
          { FastName("OnInactive"), FastName("x11") },
          { FastName("OnInactive"), FastName("x1") },
          { FastName("OnInactive"), FastName("x") },
        };

        TEST_VERIFY(expectedCallSequence == callSequence);
    }

    // UIControl::RemoveControl
    DAVA_TEST (RemoveControlFromVisibleHierarhy_OnBecomeInvisibleAndOnDisappearCalls)
    {
        root->SetVisibilityFlag(true);
        root->AddControl(x);
        callSequence.clear();

        root->RemoveControl(x);

        Vector<std::pair<FastName, FastName>> expectedCallSequence =
        {
          { FastName("OnInvisible"), FastName("x3") },
          { FastName("OnInvisible"), FastName("x21") },
          { FastName("OnInvisible"), FastName("x2") },
          { FastName("OnInvisible"), FastName("x11") },
          { FastName("OnInvisible"), FastName("x1") },
          { FastName("OnInvisible"), FastName("x") },

          { FastName("OnInactive"), FastName("x3") },
          { FastName("OnInactive"), FastName("x21") },
          { FastName("OnInactive"), FastName("x2") },
          { FastName("OnInactive"), FastName("x11") },
          { FastName("OnInactive"), FastName("x1") },
          { FastName("OnInactive"), FastName("x") },
        };

        TEST_VERIFY(expectedCallSequence == callSequence);
    }

    // UIControl::RemoveControl
    DAVA_TEST (RemoveControlInOnAppearCallbackToInvisibleHierarhy_OnAppearCalls)
    {
        root->SetVisibilityFlag(false);
        x->AddControl(z);
        callSequence.clear();

        x1->onActiveCallback = [this]()
        {
            x->RemoveControl(z);
        };

        root->AddControl(x);

        Vector<std::pair<FastName, FastName>> expectedCallSequence =
        {
          { FastName("OnActive"), FastName("x") },
          { FastName("OnActive"), FastName("x1") },
          { FastName("OnActive"), FastName("x11") },
          { FastName("OnActive"), FastName("x2") },
          { FastName("OnActive"), FastName("x21") },
          { FastName("OnActive"), FastName("x3") },
        };

        TEST_VERIFY(expectedCallSequence == callSequence);

        x1->onActiveCallback = nullptr;
    }

    // UIControl::RemoveControl
    DAVA_TEST (RemoveControlInOnDisappearCallbackToInvisibleHierarhy_OnAppearCalls)
    {
        root->SetVisibilityFlag(false);
        x->AddControl(z);
        root->AddControl(x);
        callSequence.clear();

        x1->onInactiveCallback = [this]()
        {
            x->RemoveControl(z);
        };

        root->RemoveControl(x);

        Vector<std::pair<FastName, FastName>> expectedCallSequence =
        {
          { FastName("OnInactive"), FastName("z3") },
          { FastName("OnInactive"), FastName("z2") },
          { FastName("OnInactive"), FastName("z1") },
          { FastName("OnInactive"), FastName("z") },
          { FastName("OnInactive"), FastName("x3") },
          { FastName("OnInactive"), FastName("x21") },
          { FastName("OnInactive"), FastName("x2") },
          { FastName("OnInactive"), FastName("x11") },
          { FastName("OnInactive"), FastName("x1") },
          { FastName("OnInactive"), FastName("x") },
        };

        TEST_VERIFY(expectedCallSequence == callSequence);

        x1->onInactiveCallback = nullptr;
    }

    // UIControl::RemoveControl
    DAVA_TEST (RemoveControlInOnBecomeVisibleCallbackToVisibleHierarhy_OnAppearAndBecomeVisibleCalls)
    {
        x->AddControl(z);
        callSequence.clear();
        x1->onVisibleCallback = [this]()
        {
            x->RemoveControl(z);
        };

        root->AddControl(x);

        Vector<std::pair<FastName, FastName>> expectedCallSequence =
        {
          { FastName("OnActive"), FastName("x") },
          { FastName("OnActive"), FastName("x1") },
          { FastName("OnActive"), FastName("x11") },
          { FastName("OnActive"), FastName("x2") },
          { FastName("OnActive"), FastName("x21") },
          { FastName("OnActive"), FastName("x3") },
          { FastName("OnActive"), FastName("z") },
          { FastName("OnActive"), FastName("z1") },
          { FastName("OnActive"), FastName("z2") },
          { FastName("OnActive"), FastName("z3") },

          { FastName("OnVisible"), FastName("x") },
          { FastName("OnVisible"), FastName("x1") },
          { FastName("OnInactive"), FastName("z3") },
          { FastName("OnInactive"), FastName("z2") },
          { FastName("OnInactive"), FastName("z1") },
          { FastName("OnInactive"), FastName("z") },
          { FastName("OnVisible"), FastName("x11") },
          { FastName("OnVisible"), FastName("x2") },
          { FastName("OnVisible"), FastName("x21") },
          { FastName("OnVisible"), FastName("x3") },
        };

        TEST_VERIFY(expectedCallSequence == callSequence);

        x1->onVisibleCallback = nullptr;
    }

    // UIControl::RemoveControl
    DAVA_TEST (RemoveControlInOnBecomeInvisibleCallbackToVisibleHierarhy_OnDisappearAndBecomeInvisibleCalls)
    {
        x->AddControl(z);
        root->AddControl(x);
        callSequence.clear();
        x1->onInvisibleCallback = [this]()
        {
            x->RemoveControl(z);
        };

        x->RemoveFromParent();

        Vector<std::pair<FastName, FastName>> expectedCallSequence =
        {
          { FastName("OnInvisible"), FastName("z3") },
          { FastName("OnInvisible"), FastName("z2") },
          { FastName("OnInvisible"), FastName("z1") },
          { FastName("OnInvisible"), FastName("z") },
          { FastName("OnInvisible"), FastName("x3") },
          { FastName("OnInvisible"), FastName("x21") },
          { FastName("OnInvisible"), FastName("x2") },
          { FastName("OnInvisible"), FastName("x11") },
          { FastName("OnInvisible"), FastName("x1") },
          { FastName("OnInactive"), FastName("z3") },
          { FastName("OnInactive"), FastName("z2") },
          { FastName("OnInactive"), FastName("z1") },
          { FastName("OnInactive"), FastName("z") },
          { FastName("OnInvisible"), FastName("x") },

          { FastName("OnInactive"), FastName("x3") },
          { FastName("OnInactive"), FastName("x21") },
          { FastName("OnInactive"), FastName("x2") },
          { FastName("OnInactive"), FastName("x11") },
          { FastName("OnInactive"), FastName("x1") },
          { FastName("OnInactive"), FastName("x") },
        };

        TEST_VERIFY(expectedCallSequence == callSequence);

        x1->onInvisibleCallback = nullptr;
    }

    // UIControl::SetVisibilityFlag
    DAVA_TEST (SetVisibleForInvisibleRoot_OnlyOnBecomeVisibleCalls)
    {
        x->SetVisibilityFlag(false);
        root->AddControl(x);
        callSequence.clear();

        x->SetVisibilityFlag(true);

        Vector<std::pair<FastName, FastName>> expectedCallSequence =
        {
          { FastName("OnVisible"), FastName("x") },
          { FastName("OnVisible"), FastName("x1") },
          { FastName("OnVisible"), FastName("x11") },
          { FastName("OnVisible"), FastName("x2") },
          { FastName("OnVisible"), FastName("x21") },
          { FastName("OnVisible"), FastName("x3") },
        };

        TEST_VERIFY(expectedCallSequence == callSequence);
    }

    // UIControl::SetVisibilityFlag
    DAVA_TEST (SetVisibleForVisibleControl_NothingCalls)
    {
        x->SetVisibilityFlag(true);

        root->AddControl(x);
        callSequence.clear();

        x->SetVisibilityFlag(true);

        Vector<std::pair<FastName, FastName>> expectedCallSequence;

        TEST_VERIFY(expectedCallSequence == callSequence);
    }

    // UIControl::SetVisibilityFlag
    DAVA_TEST (SetInvisibleForVisibleControl_OnlyOnBecomeInvisibleCalls)
    {
        x->SetVisibilityFlag(true);

        root->AddControl(x);
        callSequence.clear();

        x->SetVisibilityFlag(false);

        Vector<std::pair<FastName, FastName>> expectedCallSequence =
        {
          { FastName("OnInvisible"), FastName("x3") },
          { FastName("OnInvisible"), FastName("x21") },
          { FastName("OnInvisible"), FastName("x2") },
          { FastName("OnInvisible"), FastName("x11") },
          { FastName("OnInvisible"), FastName("x1") },
          { FastName("OnInvisible"), FastName("x") },
        };

        TEST_VERIFY(expectedCallSequence == callSequence);
    }

    // UIControl::SetVisibilityFlag
    DAVA_TEST (SetInvisibleForInvisibleControl_NothingCalls)
    {
        x->SetVisibilityFlag(false);

        root->AddControl(x);
        callSequence.clear();

        x->SetVisibilityFlag(false);

        Vector<std::pair<FastName, FastName>> expectedCallSequence;

        TEST_VERIFY(expectedCallSequence == callSequence);
    }

    // UIControl::SetVisibilityFlag
    DAVA_TEST (SetVisibleForNoneActiveHierarhy_NothingCalls)
    {
        x->SetVisibilityFlag(false);
        callSequence.clear();

        x->SetVisibilityFlag(true);

        TEST_VERIFY(callSequence.empty());
    }

    // UIControl::SetVisibilityFlag
    DAVA_TEST (SetInvisibleForNotActiveHierarhy_NothingCalls)
    {
        x->SetVisibilityFlag(false);

        TEST_VERIFY(callSequence.empty());
    }

    // UIControl::SetVisibilityFlag
    DAVA_TEST (SetVisibleInOnAppearCallbackToInvisibleHierarhy_OnAppearCalls)
    {
        root->SetVisibilityFlag(false);
        x->AddControl(z);
        z->SetVisibilityFlag(false);
        callSequence.clear();
        x1->onActiveCallback = [this]()
        {
            z->SetVisibilityFlag(true);
        };

        root->AddControl(x);

        Vector<std::pair<FastName, FastName>> expectedCallSequence =
        {
          { FastName("OnActive"), FastName("x") },
          { FastName("OnActive"), FastName("x1") },
          { FastName("OnActive"), FastName("x11") },
          { FastName("OnActive"), FastName("x2") },
          { FastName("OnActive"), FastName("x21") },
          { FastName("OnActive"), FastName("x3") },
          { FastName("OnActive"), FastName("z") },
          { FastName("OnActive"), FastName("z1") },
          { FastName("OnActive"), FastName("z2") },
          { FastName("OnActive"), FastName("z3") },
        };

        TEST_VERIFY(expectedCallSequence == callSequence);

        x1->onActiveCallback = nullptr;
    }

    // UIControl::SetVisibilityFlag
    DAVA_TEST (SetVisibleInOnDisappearCallbackToInvisibleHierarhy_OnDisappearCallsOnlyForXControlOriginalChildren)
    {
        root->SetVisibilityFlag(false);
        root->AddControl(x);
        x->AddControl(z);
        z->SetVisibilityFlag(false);
        callSequence.clear();
        x1->onInactiveCallback = [this]()
        {
            z->SetVisibilityFlag(true);
        };

        x->RemoveFromParent();

        Vector<std::pair<FastName, FastName>> expectedCallSequence =
        {
          { FastName("OnInactive"), FastName("z3") },
          { FastName("OnInactive"), FastName("z2") },
          { FastName("OnInactive"), FastName("z1") },
          { FastName("OnInactive"), FastName("z") },
          { FastName("OnInactive"), FastName("x3") },
          { FastName("OnInactive"), FastName("x21") },
          { FastName("OnInactive"), FastName("x2") },
          { FastName("OnInactive"), FastName("x11") },
          { FastName("OnInactive"), FastName("x1") },
          { FastName("OnInactive"), FastName("x") },
        };

        TEST_VERIFY(expectedCallSequence == callSequence);

        x1->onInactiveCallback = nullptr;
    }

    // UIControl::SetVisibilityFlag
    DAVA_TEST (SetVisibleInOnBecomeVisibleCallbackToVisibleHierarhy_OnAppearAndBecomeVisibleCalls)
    {
        x->AddControl(z);
        z->SetVisibilityFlag(false);
        callSequence.clear();

        x1->onVisibleCallback = [this]()
        {
            z->SetVisibilityFlag(true);
        };

        root->AddControl(x);

        Vector<std::pair<FastName, FastName>> expectedCallSequence =
        {
          { FastName("OnActive"), FastName("x") },
          { FastName("OnActive"), FastName("x1") },
          { FastName("OnActive"), FastName("x11") },
          { FastName("OnActive"), FastName("x2") },
          { FastName("OnActive"), FastName("x21") },
          { FastName("OnActive"), FastName("x3") },
          { FastName("OnActive"), FastName("z") },
          { FastName("OnActive"), FastName("z1") },
          { FastName("OnActive"), FastName("z2") },
          { FastName("OnActive"), FastName("z3") },

          { FastName("OnVisible"), FastName("x") },
          { FastName("OnVisible"), FastName("x1") },
          { FastName("OnVisible"), FastName("z") },
          { FastName("OnVisible"), FastName("z1") },
          { FastName("OnVisible"), FastName("z2") },
          { FastName("OnVisible"), FastName("z3") },
          { FastName("OnVisible"), FastName("x11") },
          { FastName("OnVisible"), FastName("x2") },
          { FastName("OnVisible"), FastName("x21") },
          { FastName("OnVisible"), FastName("x3") },
        };

        TEST_VERIFY(expectedCallSequence == callSequence);

        x1->onVisibleCallback = nullptr;
    }

    // UIControl::SetVisibilityFlag
    DAVA_TEST (SetVisibleInOnBecomeInvisibleCallbackToVisibleHierarhy_OnDisappearAndBecomeInvisibleCalls)
    {
        root->AddControl(x);
        x->AddControl(z);
        z->SetVisibilityFlag(false);
        callSequence.clear();
        x1->onInvisibleCallback = [this]()
        {
            z->SetVisibilityFlag(true);
        };

        x->RemoveFromParent();

        Vector<std::pair<FastName, FastName>> expectedCallSequence =
        {
          { FastName("OnInvisible"), FastName("x3") },
          { FastName("OnInvisible"), FastName("x21") },
          { FastName("OnInvisible"), FastName("x2") },
          { FastName("OnInvisible"), FastName("x11") },
          { FastName("OnInvisible"), FastName("x1") },
          { FastName("OnVisible"), FastName("z") },
          { FastName("OnVisible"), FastName("z1") },
          { FastName("OnVisible"), FastName("z2") },
          { FastName("OnVisible"), FastName("z3") },
          { FastName("OnInvisible"), FastName("x") },
          { FastName("OnInvisible"), FastName("z3") },
          { FastName("OnInvisible"), FastName("z2") },
          { FastName("OnInvisible"), FastName("z1") },
          { FastName("OnInvisible"), FastName("z") },

          { FastName("OnInactive"), FastName("z3") },
          { FastName("OnInactive"), FastName("z2") },
          { FastName("OnInactive"), FastName("z1") },
          { FastName("OnInactive"), FastName("z") },
          { FastName("OnInactive"), FastName("x3") },
          { FastName("OnInactive"), FastName("x21") },
          { FastName("OnInactive"), FastName("x2") },
          { FastName("OnInactive"), FastName("x11") },
          { FastName("OnInactive"), FastName("x1") },
          { FastName("OnInactive"), FastName("x") },
        };

        TEST_VERIFY(expectedCallSequence == callSequence);

        x1->onInvisibleCallback = nullptr;
    }

    // UIControl::SetVisibilityFlag
    DAVA_TEST (SetInvisibleInOnAppearCallbackToInvisibleHierarhy_OnAppearCalls) //
    {
        root->SetVisibilityFlag(false);
        x->AddControl(z);
        z->SetVisibilityFlag(false);
        callSequence.clear();
        x1->onActiveCallback = [this]()
        {
            z->SetVisibilityFlag(false);
        };

        root->AddControl(x);

        Vector<std::pair<FastName, FastName>> expectedCallSequence =
        {
          { FastName("OnActive"), FastName("x") },
          { FastName("OnActive"), FastName("x1") },
          { FastName("OnActive"), FastName("x11") },
          { FastName("OnActive"), FastName("x2") },
          { FastName("OnActive"), FastName("x21") },
          { FastName("OnActive"), FastName("x3") },
          { FastName("OnActive"), FastName("z") },
          { FastName("OnActive"), FastName("z1") },
          { FastName("OnActive"), FastName("z2") },
          { FastName("OnActive"), FastName("z3") },
        };

        TEST_VERIFY(expectedCallSequence == callSequence);

        x1->onActiveCallback = nullptr;
    }

    // UIControl::SetVisibilityFlag
    DAVA_TEST (SetInvisibleInOnDisappearCallbackToInvisibleHierarhy_OnDisappearCallsOnlyForXControlOriginalChildren)
    {
        root->SetVisibilityFlag(false);
        root->AddControl(x);
        x->AddControl(z);
        z->SetVisibilityFlag(false);
        callSequence.clear();
        x1->onInactiveCallback = [this]()
        {
            z->SetVisibilityFlag(false);
        };

        x->RemoveFromParent();

        Vector<std::pair<FastName, FastName>> expectedCallSequence =
        {
          { FastName("OnInactive"), FastName("z3") },
          { FastName("OnInactive"), FastName("z2") },
          { FastName("OnInactive"), FastName("z1") },
          { FastName("OnInactive"), FastName("z") },
          { FastName("OnInactive"), FastName("x3") },
          { FastName("OnInactive"), FastName("x21") },
          { FastName("OnInactive"), FastName("x2") },
          { FastName("OnInactive"), FastName("x11") },
          { FastName("OnInactive"), FastName("x1") },
          { FastName("OnInactive"), FastName("x") },
        };

        TEST_VERIFY(expectedCallSequence == callSequence);

        x1->onInactiveCallback = nullptr;
    }

    // UIControl::SetVisibilityFlag
    DAVA_TEST (SetInvisibleInOnBecomeVisibleCallbackToVisibleHierarhy_OnAppearAndBecomeVisibleCalls)
    {
        x->AddControl(z);
        callSequence.clear();

        x1->onVisibleCallback = [this]()
        {
            z->SetVisibilityFlag(false);
        };

        root->AddControl(x);

        Vector<std::pair<FastName, FastName>> expectedCallSequence =
        {
          { FastName("OnActive"), FastName("x") },
          { FastName("OnActive"), FastName("x1") },
          { FastName("OnActive"), FastName("x11") },
          { FastName("OnActive"), FastName("x2") },
          { FastName("OnActive"), FastName("x21") },
          { FastName("OnActive"), FastName("x3") },
          { FastName("OnActive"), FastName("z") },
          { FastName("OnActive"), FastName("z1") },
          { FastName("OnActive"), FastName("z2") },
          { FastName("OnActive"), FastName("z3") },

          { FastName("OnVisible"), FastName("x") },
          { FastName("OnVisible"), FastName("x1") },
          { FastName("OnVisible"), FastName("x11") },
          { FastName("OnVisible"), FastName("x2") },
          { FastName("OnVisible"), FastName("x21") },
          { FastName("OnVisible"), FastName("x3") },
        };

        TEST_VERIFY(expectedCallSequence == callSequence);

        x1->onVisibleCallback = nullptr;
    }

    // UIControl::SetVisibilityFlag
    DAVA_TEST (SetInvisibleInOnBecomeInvisibleCallbackToVisibleHierarhy_OnDisappearAndBecomeInvisibleCalls)
    {
        root->AddControl(x);
        x->AddControl(z);
        callSequence.clear();
        x1->onInvisibleCallback = [this]()
        {
            z->SetVisibilityFlag(false);
        };

        x->RemoveFromParent();

        Vector<std::pair<FastName, FastName>> expectedCallSequence =
        {
          { FastName("OnInvisible"), FastName("z3") },
          { FastName("OnInvisible"), FastName("z2") },
          { FastName("OnInvisible"), FastName("z1") },
          { FastName("OnInvisible"), FastName("z") },
          { FastName("OnInvisible"), FastName("x3") },
          { FastName("OnInvisible"), FastName("x21") },
          { FastName("OnInvisible"), FastName("x2") },
          { FastName("OnInvisible"), FastName("x11") },
          { FastName("OnInvisible"), FastName("x1") },
          { FastName("OnInvisible"), FastName("x") },

          { FastName("OnInactive"), FastName("z3") },
          { FastName("OnInactive"), FastName("z2") },
          { FastName("OnInactive"), FastName("z1") },
          { FastName("OnInactive"), FastName("z") },
          { FastName("OnInactive"), FastName("x3") },
          { FastName("OnInactive"), FastName("x21") },
          { FastName("OnInactive"), FastName("x2") },
          { FastName("OnInactive"), FastName("x11") },
          { FastName("OnInactive"), FastName("x1") },
          { FastName("OnInactive"), FastName("x") },
        };

        TEST_VERIFY(expectedCallSequence == callSequence);

        x1->onInvisibleCallback = nullptr;
    }

    class UITestControl : public UIControl
    {
    public:
        UITestControl()
        {
        }

        DAVA::Function<void()> onVisibleCallback;
        DAVA::Function<void()> onInvisibleCallback;
        DAVA::Function<void()> onActiveCallback;
        DAVA::Function<void()> onInactiveCallback;

    protected:
        ~UITestControl() override
        {
        }

        void OnVisible() override
        {
            UIControl::OnVisible();
            callSequence.emplace_back(FastName("OnVisible"), GetName());
            if (onVisibleCallback)
                onVisibleCallback();
        }
        void OnInvisible() override
        {
            UIControl::OnInvisible();
            callSequence.emplace_back(FastName("OnInvisible"), GetName());
            if (onInvisibleCallback)
                onInvisibleCallback();
        }

        void OnActive() override
        {
            UIControl::OnActive();
            callSequence.emplace_back(FastName("OnActive"), GetName());
            if (onActiveCallback)
                onActiveCallback();
        }
        void OnInactive() override
        {
            UIControl::OnInactive();
            callSequence.emplace_back(FastName("OnInactive"), GetName());
            if (onInactiveCallback)
                onInactiveCallback();
        }
    };
};
Vector<std::pair<FastName, FastName>> UIControlHierarhyTest::callSequence;
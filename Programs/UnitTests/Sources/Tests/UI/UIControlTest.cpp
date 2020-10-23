#include "DAVAEngine.h"

#include "UI/UIControlPackageContext.h"
#include "UnitTests/UnitTests.h"

using namespace DAVA;

DAVA_TESTCLASS (UIControlTest)
{
    // root
    // |-a
    // | |-1
    // | | |-1
    // | |
    // | |-2
    // | | |-1
    // | |
    // | |-3
    // |
    // |-b
    // | |-1
    // | | |-1
    // | | |-2
    // | |
    // | |-2
    // | | |-1
    // | |
    // | |-3
    // |
    // |-c
    // | |-1
    // | |-2
    // | |-3

    UIControl* root = nullptr;
    UIControl* a = nullptr;
    UIControl* a1 = nullptr;
    UIControl* a2 = nullptr;
    UIControl* a3 = nullptr;
    UIControl* a11 = nullptr;
    UIControl* a21 = nullptr;

    UIControl* b = nullptr;
    UIControl* b1 = nullptr;
    UIControl* b2 = nullptr;
    UIControl* b3 = nullptr;
    UIControl* b11 = nullptr;
    UIControl* b12 = nullptr;
    UIControl* b21 = nullptr;

    UIControl* c = nullptr;
    UIControl* c1 = nullptr;
    UIControl* c2 = nullptr;
    UIControl* c3 = nullptr;

    UIControl* MakeRoot(const char* name)
    {
        UIControl* c = new UIControl();
        c->SetName(name);
        return c;
    }

    UIControl* MakeChild(UIControl * parent, const char* name)
    {
        UIControl* c = new UIControl();
        c->SetName(name);
        parent->AddControl(c);
        return c;
    }

    void SetUp(const String& testName) override
    {
        root = MakeRoot("root");

        a = MakeChild(root, "a");
        a1 = MakeChild(a, "1");
        a2 = MakeChild(a, "2");
        a3 = MakeChild(a, "3");
        a11 = MakeChild(a1, "1");
        a21 = MakeChild(a2, "1");

        b = MakeChild(root, "b");
        b1 = MakeChild(b, "1");
        b2 = MakeChild(b, "2");
        b3 = MakeChild(b, "3");
        b11 = MakeChild(b1, "1");
        b12 = MakeChild(b1, "2");
        b21 = MakeChild(b2, "1");

        c = MakeChild(root, "c");
        c1 = MakeChild(c, "1");
        c2 = MakeChild(c, "2");
        c3 = MakeChild(c, "3");
    }

    void TearDown(const String& testName) override
    {
        SafeRelease(root);
        SafeRelease(a);
        SafeRelease(a1);
        SafeRelease(a2);
        SafeRelease(a3);
        SafeRelease(a11);
        SafeRelease(a21);

        SafeRelease(b);
        SafeRelease(b1);
        SafeRelease(b2);
        SafeRelease(b3);
        SafeRelease(b11);
        SafeRelease(b12);
        SafeRelease(b21);

        SafeRelease(c);
        SafeRelease(c1);
        SafeRelease(c2);
        SafeRelease(c3);
    }

    // UIControl::FindByName
    DAVA_TEST (FindThemSelves)
    {
        TEST_VERIFY(root->FindByPath(".") == root);
        TEST_VERIFY(b->FindByPath(".") == b);
    }

    // UIControl::FindByName
    DAVA_TEST (FindLocalRoot)
    {
        TEST_VERIFY(root->FindByPath("^") == nullptr);
        TEST_VERIFY(c1->FindByPath("^") == nullptr);

        RefPtr<UIControlPackageContext> context = MakeRef<UIControlPackageContext>();
        root->SetPackageContext(context);

        TEST_VERIFY(root->FindByPath("^") == nullptr);
        TEST_VERIFY(c1->FindByPath("^") == root);
        TEST_VERIFY(b21->FindByPath("^") == root);
    }

    // UIControl::FindByName
    DAVA_TEST (FindParent)
    {
        TEST_VERIFY(root->FindByPath("..") == nullptr);
        TEST_VERIFY(c->FindByPath("..") == root);
        TEST_VERIFY(c1->FindByPath("..") == c);
        TEST_VERIFY(b21->FindByPath("..") == b2);
    }

    // UIControl::FindByName
    DAVA_TEST (MatchesOneLevel)
    {
        TEST_VERIFY(root->FindByPath("*/1") == a1);
        TEST_VERIFY(root->FindByPath("*/*/1") == a11);
        TEST_VERIFY(root->FindByPath("*/a") == nullptr);
        TEST_VERIFY(root->FindByPath("*/root") == nullptr);
    }

    // UIControl::FindByName
    DAVA_TEST (MatchesZeroOrMoreLevels)
    {
        TEST_VERIFY(root->FindByPath("**/1") == a1);
        TEST_VERIFY(root->FindByPath("**/a") == a);
        TEST_VERIFY(root->FindByPath("**/root") == nullptr);
        TEST_VERIFY(root->FindByPath("**/1/1") == a11);
    }

    // UIControl::FindByName
    DAVA_TEST (FindSomePatches)
    {
        TEST_VERIFY(root->FindByPath("b/2/1") == b21);
        TEST_VERIFY(b11->FindByPath("../..") == b);
        TEST_VERIFY(b11->FindByPath("../2/../../2/1") == b21);
    }
};
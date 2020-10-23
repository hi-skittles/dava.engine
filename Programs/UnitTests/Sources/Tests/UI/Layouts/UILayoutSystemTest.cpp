#include "DAVAEngine.h"

#include "UI/UIControl.h"
#include "UI/Layouts/UILayoutSystem.h"
#include "UI/Layouts/UIAnchorComponent.h"
#include "UI/Layouts/UISizePolicyComponent.h"

#include "UnitTests/UnitTests.h"

using namespace DAVA;

DAVA_TESTCLASS (UILayoutSystemTest)
{
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

    DAVA_TEST (QuickPositionLayout_LayoutsPositionsWithoutSize)
    {
        UIControl* screen = MakeRoot("screen");
        screen->SetSize(Vector2(200.0f, 200.0f));

        UIControl* parent = MakeChild(screen, "parent");
        UISizePolicyComponent* parentSizePolicy = parent->GetOrCreateComponent<UISizePolicyComponent>();
        parentSizePolicy->SetHorizontalPolicy(UISizePolicyComponent::PERCENT_OF_FIRST_CHILD);
        parentSizePolicy->SetHorizontalValue(100.0f);

        UIAnchorComponent* parentAnchor = parent->GetOrCreateComponent<UIAnchorComponent>();
        parentAnchor->SetHCenterAnchorEnabled(true);

        UIControl* child = MakeChild(parent, "child");
        UISizePolicyComponent* childSizePolicy = child->GetOrCreateComponent<UISizePolicyComponent>();
        childSizePolicy->SetHorizontalPolicy(UISizePolicyComponent::FIXED_SIZE);
        childSizePolicy->SetHorizontalValue(100.0f);

        GetEngineContext()->uiControlSystem->GetLayoutSystem()->ProcessControl(screen);
        TEST_VERIFY(FLOAT_EQUAL_EPS(parent->GetPosition().x, 50.0f, 0.01f));

        parent->SetPosition(Vector2(0.0f, 0.0f));
        GetEngineContext()->uiControlSystem->GetLayoutSystem()->ProcessControl(parent);
        TEST_VERIFY(FLOAT_EQUAL_EPS(parent->GetPosition().x, 50.0f, 0.01f));

        SafeRelease(screen);
        SafeRelease(parent);
        SafeRelease(child);
    }
};

#include "Infrastructure/TestListScreen.h"
#include <Utils/UTF8Utils.h>

#include <UI/Layouts/UISizePolicyComponent.h>
#include <UI/Render/UIDebugRenderComponent.h>

using namespace DAVA;

TestListScreen::~TestListScreen()
{
    for (auto screen : testScreens)
    {
        SafeRelease(screen);
    }
    testScreens.clear();
}

void TestListScreen::AddTestScreen(BaseScreen* screen)
{
    screen->Retain();
    testScreens.push_back(screen);
}

void TestListScreen::LoadResources()
{
    UIScreen::LoadResources();

    Size2i screenSize = GetEngineContext()->uiControlSystem->vcs->GetVirtualScreenSize();

    testsGrid = new UIList(Rect(), UIList::ORIENTATION_VERTICAL);
    testsGrid->SetDelegate(this);
    testsGrid->GetOrCreateComponent<UIDebugRenderComponent>();

    UISizePolicyComponent* sizePolicy = testsGrid->GetOrCreateComponent<UISizePolicyComponent>();
    sizePolicy->SetHorizontalPolicy(UISizePolicyComponent::PERCENT_OF_PARENT);
    sizePolicy->SetVerticalPolicy(UISizePolicyComponent::PERCENT_OF_PARENT);
    sizePolicy->SetHorizontalValue(100.0f);
    sizePolicy->SetVerticalValue(100.0f);

    AddControl(testsGrid);
}

void TestListScreen::UnloadResources()
{
    UIScreen::UnloadResources();
    RemoveAllControls();

    SafeRelease(testsGrid);
}

int32 TestListScreen::ElementsCount(UIList* list)
{
    return static_cast<int32>(testScreens.size());
}

float32 TestListScreen::CellHeight(UIList* list, int32 index)
{
    return cellHeight;
}

UIListCell* TestListScreen::CellAtIndex(UIList* list, int32 index)
{
    const char8 buttonName[] = "CellButton";
    const char8 cellName[] = "TestButtonCell";
    UIStaticText* buttonText = nullptr;
    UIListCell* c = list->GetReusableCell(cellName); //try to get cell from the reusable cells store
    if (c == nullptr)
    { //if cell of requested type isn't find in the store create new cell
        c = new UIListCell(Rect(), cellName);
        c->GetOrCreateComponent<UIDebugRenderComponent>();
        {
            UISizePolicyComponent* sizePolicy = c->GetOrCreateComponent<UISizePolicyComponent>();
            sizePolicy->SetHorizontalPolicy(UISizePolicyComponent::PERCENT_OF_PARENT);
            sizePolicy->SetHorizontalValue(100.0f);
        }

        buttonText = new UIStaticText(Rect(0., 0., static_cast<float32>(list->size.x), CellHeight(list, index)));
        buttonText->SetName(buttonName);
        c->AddControl(buttonText);

        Font* font = FTFont::Create("~res:/TestBed/Fonts/korinna.ttf");
        DVASSERT(font);

        buttonText->SetFont(font);
        buttonText->SetFontSize(20.f);
        buttonText->GetOrCreateComponent<UIDebugRenderComponent>();
        {
            UISizePolicyComponent* sizePolicy = buttonText->GetOrCreateComponent<UISizePolicyComponent>();
            sizePolicy->SetHorizontalPolicy(UISizePolicyComponent::PERCENT_OF_PARENT);
            sizePolicy->SetHorizontalValue(100.0f);
        }

        SafeRelease(font);
        UIControlBackground* background = c->GetOrCreateComponent<UIControlBackground>();
        background->SetColor(Color(0.75, 0.75, 0.75, 0.5));
    }

    auto screen = testScreens.at(index);

    buttonText = static_cast<UIStaticText*>(c->FindByName(buttonName));
    if (nullptr != buttonText)
    {
        buttonText->SetText(UTF8Utils::EncodeToWideString(screen->GetName().c_str()));
    }

    return c; //returns cell
}

void TestListScreen::OnCellSelected(UIList* forList, UIListCell* selectedCell)
{
    GetEngineContext()->uiScreenManager->SetScreen(selectedCell->GetIndex() + 1);
}

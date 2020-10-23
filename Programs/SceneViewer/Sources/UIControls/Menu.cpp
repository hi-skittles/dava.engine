#include "Menu.h"

void MenuItem::SetEnabled(bool enabled)
{
    button->SetState(enabled ? DAVA::UIControl::eControlState::STATE_NORMAL : DAVA::UIControl::eControlState::STATE_DISABLED);
    button->SetInputEnabled(enabled, false);
}

ActionItem::ActionItem(Menu* parentMenu, DAVA::Message& action)
    : parentMenu(parentMenu)
    , action(action)
{
}

void ActionItem::OnActivate(DAVA::BaseObject* caller, void* param, void* callerData)
{
    if (parentMenu)
    {
        parentMenu->BackToMainMenu();
    }
    action(caller, callerData);
}

namespace MenuDetails
{
const DAVA::float32 SPACE_BETWEEN_BUTTONS = 10.0f;
const DAVA::Color GRAY(0.6f, 0.6f, 0.6f, 1.f);
}

Menu::Menu(Menu* parentMenu, DAVA::UIControl* bearerControl, DAVA::ScopedPtr<DAVA::Font>& font, DAVA::Rect& firstButtonRect)
    : parentMenu(parentMenu)
    , bearerControl(bearerControl)
    , font(font)
    , firstButtonRect(firstButtonRect)
    , nextButtonRect(firstButtonRect)
{
}

Menu::~Menu()
{
    for (std::unique_ptr<MenuItem>& item : menuItems)
    {
        bearerControl->RemoveControl(item->button);
    }
}

ActionItem* Menu::AddActionItem(const DAVA::WideString& text, DAVA::Message action)
{
    ActionItem* actionItem = new ActionItem(this, action);
    menuItems.emplace_back(actionItem);
    actionItem->button = ConstructMenuButton(text, DAVA::Message(actionItem, &ActionItem::OnActivate));
    return actionItem;
}

SubMenuItem* Menu::AddSubMenuItem(const DAVA::WideString& text)
{
    SubMenuItem* subMenuItem = new SubMenuItem;
    menuItems.emplace_back(subMenuItem);

    subMenuItem->submenu.reset(new Menu(this, bearerControl, font, firstButtonRect));
    subMenuItem->button = ConstructMenuButton(text, DAVA::Message(subMenuItem->submenu.get(), &Menu::OnActivate));

    return subMenuItem;
}

void Menu::AddBackItem()
{
    menuItems.emplace_back(new MenuItem);
    menuItems.back()->button = ConstructMenuButton(L"Back", DAVA::Message(this, &Menu::OnBack));
}

void Menu::BackToMainMenu()
{
    if (parentMenu)
    {
        Show(false);
        parentMenu->BackToMainMenu();
    }
    else
    {
        Show(true);
    }
}

DAVA::ScopedPtr<DAVA::UIButton> Menu::ConstructMenuButton(const DAVA::WideString& text, const DAVA::Message& action)
{
    using namespace DAVA;

    DAVA::ScopedPtr<DAVA::UIButton> button(new DAVA::UIButton(nextButtonRect));
    nextButtonRect.y += (nextButtonRect.dy + MenuDetails::SPACE_BETWEEN_BUTTONS);

    button->SetVisibilityFlag(IsFirstLevelMenu());
    button->SetStateText(DAVA::UIControl::STATE_NORMAL, text);

    button->SetStateTextAlign(DAVA::UIControl::STATE_NORMAL, DAVA::ALIGN_HCENTER | DAVA::ALIGN_VCENTER);
    button->SetStateFontColor(DAVA::UIControl::STATE_NORMAL, DAVA::Color::White);
    button->SetStateFont(DAVA::UIControl::STATE_NORMAL, font);
    button->SetStateTextColorInheritType(DAVA::UIControl::STATE_NORMAL, DAVA::UIControlBackground::COLOR_IGNORE_PARENT);

    button->SetStateFontColor(DAVA::UIControl::STATE_DISABLED, MenuDetails::GRAY);

    button->SetStateDrawType(UIControl::STATE_NORMAL, UIControlBackground::DRAW_FILL);
    button->GetStateBackground(UIControl::STATE_NORMAL)->SetColor(Color(0.4f, 0.5f, 0.4f, 0.9f));
    button->SetStateDrawType(UIControl::STATE_PRESSED_INSIDE, UIControlBackground::DRAW_FILL);
    button->GetStateBackground(UIControl::STATE_PRESSED_INSIDE)->SetColor(Color(0.65f, 0.75f, 0.65f, 0.9f));

    button->AddEvent(DAVA::UIControl::EVENT_TOUCH_UP_INSIDE, action);
    bearerControl->AddControl(button);

    return button;
}

void Menu::BringAtFront()
{
    for (std::unique_ptr<MenuItem>& menuItem : menuItems)
    {
        bearerControl->BringChildFront(menuItem->button);

        SubMenuItem* subMenuItem = dynamic_cast<SubMenuItem*>(menuItem.get());
        if (subMenuItem != nullptr)
        {
            subMenuItem->submenu->BringAtFront();
        }
    }
}

void Menu::Show(bool toShow)
{
    for (auto& menuItem : menuItems)
    {
        menuItem->button->SetVisibilityFlag(toShow);
    }
}

void Menu::SetEnabled(bool enabled)
{
    for (auto& menuItem : menuItems)
    {
        menuItem->SetEnabled(enabled);
    }
}

void Menu::AllowInput(bool allow)
{
    for (auto& item : menuItems)
    {
        item->button->SetNoInput(!allow);
    }
}

void Menu::OnBack(DAVA::BaseObject* caller, void* param, void* callerData)
{
    Show(false);

    if (parentMenu)
    {
        parentMenu->Show(true);
    }
}

void Menu::OnActivate(DAVA::BaseObject* caller, void* param, void* callerData)
{
    if (parentMenu)
    {
        parentMenu->Show(false);
    }

    Show(true);
}

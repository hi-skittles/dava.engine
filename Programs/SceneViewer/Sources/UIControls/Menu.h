#pragma once

#include <Base/BaseTypes.h>
#include <Base/Message.h>
#include <Base/ScopedPtr.h>
#include <Base/BaseObject.h>
#include <Math/Rect.h>
#include <Math/Color.h>
#include <UI/UIButton.h>
#include <UI/UIControl.h>

struct MenuItem
{
    virtual ~MenuItem()
    {
    }
    void SetEnabled(bool);
    DAVA::ScopedPtr<DAVA::UIButton> button;
};

class Menu;

class ActionItem : public MenuItem
{
public:
    explicit ActionItem(Menu* parentMenu, DAVA::Message& action);
    void OnActivate(DAVA::BaseObject* caller, void* param, void* callerData);

private:
    Menu* parentMenu = nullptr;
    DAVA::Message action;
};

struct SubMenuItem : public MenuItem
{
    std::unique_ptr<Menu> submenu;
};

class Menu final
{
public:
    explicit Menu(Menu* parentMenu, DAVA::UIControl* bearerControl, DAVA::ScopedPtr<DAVA::Font>&, DAVA::Rect& firstButtonRect);
    ~Menu();

    void BringAtFront();
    void Show(bool toShow);
    void SetEnabled(bool enabled);
    void AllowInput(bool);

    ActionItem* AddActionItem(const DAVA::WideString& text, DAVA::Message action);
    SubMenuItem* AddSubMenuItem(const DAVA::WideString& text);
    void AddBackItem();
    void BackToMainMenu();

private:
    DAVA::ScopedPtr<DAVA::UIButton> ConstructMenuButton(const DAVA::WideString& text, const DAVA::Message& action);
    void OnBack(DAVA::BaseObject* caller, void* param, void* callerData);
    void OnActivate(DAVA::BaseObject* caller, void* param, void* callerData);
    bool IsFirstLevelMenu() const;

private:
    Menu* parentMenu = nullptr;
    DAVA::UIControl* bearerControl = nullptr;
    DAVA::ScopedPtr<DAVA::Font> font = nullptr;

    DAVA::Vector<std::unique_ptr<MenuItem>> menuItems;

    DAVA::Rect firstButtonRect;
    DAVA::Rect nextButtonRect;
};

inline bool Menu::IsFirstLevelMenu() const
{
    return (parentMenu == nullptr);
}

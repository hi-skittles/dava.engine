#pragma once

#include "Infrastructure/BaseScreen.h"
#include <UI/UIList.h>

using namespace DAVA;

class TestListScreen : public UIScreen, public UIListDelegate
{
protected:
    virtual ~TestListScreen();

public:
    TestListScreen() = default;

    void LoadResources() override;
    void UnloadResources() override;

    void AddTestScreen(BaseScreen* screen);

    //UIListDelegate interface
private:
    float32 CellHeight(UIList* list, int32 index) override;
    int32 ElementsCount(UIList* list) override;
    UIListCell* CellAtIndex(UIList* list, int32 index) override;
    void OnCellSelected(UIList* forList, UIListCell* selectedCell) override;

private:
    Vector<BaseScreen*> testScreens;
    UIList* testsGrid = nullptr;
    float32 cellHeight = 50.f;
};

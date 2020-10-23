#ifndef __DAVAENGINE_UI_LIST_H__
#define __DAVAENGINE_UI_LIST_H__

#include "Base/BaseTypes.h"
#include "UI/UIControl.h"
#include "UI/UIScrollBarDelegate.h"

namespace DAVA
{
/**
    \ingroup controlsystem
    \brief Class container for other controls and display them as list

    This class can show inner controls (UIListCell) as list.
    Inner controls are not real childs of UIList. Each List has scrollContainer which contain cells.
    All cells are build from specified by ID UIAggregatorControl.

    Aggregator ID of specific aggregator is set inside user's implementation of UIListDelegate::CellAtIndex.
    Only one aggregator for cells is allowed for a list.

    Aggregator Path is used for Save/Load procedure. Using this path, system can locate proper aggregator and get
    its ID.
*/
/**
    \ingroup controlsystem
    \brief UIList is a control for displaying lists of information on the screen.
    It's simple and powerfull. Using this class you can create list.
    Lists can be vertical also, so you can create scrollable pages easily.

    Example of UIList usage:
    \code
    //on list creation you need to set your class as the delegate
    void MultiplayerScreen::LoadResources()
    {
        serversList = new UIList(Rect(10, 45, 460, 210), UIList::ORIENTATION_VERTICAL);
        serversList->SetDelegate(this);
        AddControl(serversList);
    }

    //next method should be realized in a delegate class

    //returns total cells count in the list
    int32 MultiplayerScreen::ElementsCount(UIList *forList)
    {
        return GameServer::Instance()->totalServers.size();
    }

    //returns cell dimension for the UIList calculations
    int32 MultiplayerScreen::CellHeight(UIList *forList, int32 index)//calls only for vertical orientation
    {
        return SERVER_CELL_HEIGHT;
    }

    //create cells and fill them with data
    UIListCell *MultiplayerScreen::CellAtIndex(UIList *forList, int32 index)
    {
        GameServerCell *c = (GameServerCell *)forList->GetReusableCell("Server cell"); //try to get cell from the reusable cells store
        if(!c)
        { //if cell of requested type isn't find in the store create new cell
            c = new GameServerCell(Rect((0, 0, 200, SERVER_CELL_HEIGHT), "Server cell");
        }
        //fill cell whith data
        c->serverName = GameServer::Instance()->totalServers[index].name + LocalizedString("'s game");
        c->SetStateText(UIControl::STATE_NORMAL, c->serverName, Vector2(c->GetStateBackground(UIControl::STATE_NORMAL)->GetSprite()->GetWidth() * 1.7 - 30, 0));
        c->connection = GameServer::Instance()->totalServers[index].connection;
        c->serverIndex = GameServer::Instance()->totalServers[index].index;

        return c;//returns cell
        //your application don't need to manage cells. UIList do all cells management.
        //you can create cells of your own types derived from the UIListCell
    }

    //when cell is pressed
    void MultiplayerScreen::OnCellSelected(UIList *forList, UIListCell *selectedCell)
    {
        PlayButtonSound();

        currentName = selectedCell->serverName;
        currentConnection = selectedCell->connection;
    }
    \endcode
 */
class ScrollHelper;
class UIListDelegate;
class UIListCell;
class UIScrollBar;

class UIList : public UIControl, public UIScrollBarDelegate
{
    DAVA_VIRTUAL_REFLECTION(UIList, UIControl);

public:
    static const int32 maximumElementsCount = 100000;
    enum eListOrientation
    {
        ORIENTATION_VERTICAL = 0,
        ORIENTATION_HORIZONTAL,
    };

    UIList(const Rect& rect = Rect(), eListOrientation requiredOrientation = ORIENTATION_VERTICAL);

    void SetDelegate(UIListDelegate* newDelegate);
    UIListDelegate* GetDelegate();

    void ScrollToElement(int32 index);

    float32 GetScrollPosition();
    void SetScrollPosition(float32 newScrollPos);
    void ResetScrollPosition();
    void Refresh();

    void SetSlowDownTime(float32 newValue); //sets how fast reduce speed (for example 0.25 reduces speed to zero for the 0.25 second ). To remove inertion effect set tihs value to 0
    float32 GetSlowDownTime() const;

    void SetBorderMoveModifer(float32 newValue); //sets how scrolling element moves after reachig a border (0.5 as a default). To remove movement effect after borders set thus value to 0
    float32 GetBorderMoveModifer() const;

    void SetTouchHoldDelta(int32 holdDelta); //the amount of pixels user must move the finger on the button to switch from button to scrolling (default 30)
    int32 GetTouchHoldDelta() const;

    void ScrollTo(float32 delta);

    void ScrollToPosition(float32 position, float32 timeSec = 0.3f);

    void SetSize(const Vector2& newSize) override;

    void SetOrientation(int32 orientation);
    inline int32 GetOrientation() const
    {
        return orientation;
    };

    const List<RefPtr<UIControl>>& GetVisibleCells() const;

    UIListCell* GetReusableCell(const String& cellIdentifier); //returns cell from the cells cache, if returns 0 you need to create the new one

    void OnActive() override;

    float32 VisibleAreaSize(UIScrollBar* forScrollBar) override;
    float32 TotalAreaSize(UIScrollBar* forScrollBar) override;
    float32 ViewPosition(UIScrollBar* forScrollBar) override;
    void OnViewPositionChanged(UIScrollBar* byScrollBar, float32 newPosition) override;

    UIList* Clone() override;
    void CopyDataFrom(UIControl* srcControl) override;

    bool GetNeedRefresh();

    void ImmediateClearCells();

protected:
    void InitAfterYaml();
    virtual ~UIList();

    void FullRefresh();

    void Update(float32 timeElapsed) override;

    void Input(UIEvent* currentInput) override;
    bool SystemInput(UIEvent* currentInput) override; // Internal method used by ControlSystem
    void InputCancelled(UIEvent* currentInput) override;

    Vector<UIListCell*>* GetStoreVector(const String& cellIdentifier);
    void AddCellAtPos(UIListCell* cell, float32 pos, float32 size, int32 index);

    void OnSelectEvent(BaseObject* pCaller, void* pUserData, void* callerData);

    void RemoveCell(UIListCell* cell);
    void RemoveAllCells();
    void ClearReusableCells();

    UIListDelegate* delegate;
    eListOrientation orientation;

    UIControl* scrollContainer;

    int32 mainTouch;

    ScrollHelper* scroll;
    float32 addPos;
    float32 oldPos;
    float32 newPos;
    float32 oldScroll = 0.f;
    float32 newScroll = 0.f;

    int32 touchHoldSize;

    // Private boolean variables are grouped together because of DF-2149.
    bool lockTouch : 1;
    bool needRefresh : 1;

    Map<String, Vector<UIListCell*>*> cellStore;
};

inline bool UIList::GetNeedRefresh()
{
    return needRefresh;
}
};
#endif

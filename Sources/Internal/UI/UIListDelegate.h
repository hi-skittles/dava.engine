#pragma once
#include "Base/BaseTypes.h"

namespace DAVA
{
class YamlNode;
class UIList;
class UIListCell;
/**
    \ingroup controlsystem
    \brief UIListDelegate interface declares methods that are implemented by the delegate of UIList control.
    The methods provide data for UIList, and define it's content and allow to modify it's behaviour.
 */
class UIListDelegate
{
public:
    virtual ~UIListDelegate() = default;

private:
    friend class UIList;

    /**
        \brief This method is called by control when it need to know how many items is should display.
        Method should return number of items list in the list. It called initially when you add list and after UIList::Refresh.
        \param[in] list list object that requesting the information. You can have multiple lists with same delegate.
        \returns number of elements in the list.
     */
    virtual int32 ElementsCount(UIList* list) = 0;
    /**
        \brief This method should return UIListCell object for given index.
        \param[in] list list object that requesting the information. You can have multiple lists with same delegate.
        \param[in] index index of the list item
        \returns UIListCell that should be placed at index position in the list.
     */
    virtual UIListCell* CellAtIndex(UIList* list, int32 index) = 0;

    /**
        \brief This method is called by UIList when it need to know what is the width of the cell. It called only for horizontal lists.
        \param[in] list list object that requesting the information. You can have multiple lists with same delegate.
        \param[in] index index of the list item
        \returns width in pixels of the cell with given index. Default value is 20px.
     */
    virtual float32 CellWidth(UIList* list, int32 index); //! control calls this method only when it's in horizontal orientation

    /**
        \brief This method is called by UIList when it need to know what is the height of the cell. It called only for vertical lists.
        \param[in] list list object that requesting the information. You can have multiple lists with same delegate.
        \param[in] index index of the list item
        \returns height in pixels of the cell with given index. Default value is 20px.
     */
    virtual float32 CellHeight(UIList* list, int32 index); //control calls this method only when it's in vertical orientation

    /**
        \brief This method is called by UIList when cell was selected by user.
        \param[in] list list object that requesting the information. You can have multiple lists with same delegate.
        \param[in] index index of the list item
     */
    virtual void OnCellSelected(UIList* forList, UIListCell* selectedCell);

    /**
        \brief This metod is called by UIList when need to save.
    */
    virtual void SaveToYaml(UIList* forList, YamlNode* node);
};
};

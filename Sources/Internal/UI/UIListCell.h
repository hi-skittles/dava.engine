#ifndef __DAVAENGINE_UI_LIST_CELL_H__
#define __DAVAENGINE_UI_LIST_CELL_H__

#include "UI/UIControl.h"
#include "Reflection/Reflection.h"

namespace DAVA
{
/**
	 \ingroup controlsystem
	 \brief Cell unit for the UIList.
		UIControl that can be managed by the UIList.
	 */

class UIListCell : public UIControl
{
    friend class UIList;
    DAVA_VIRTUAL_REFLECTION(UIListCell, UIControl);

public:
    /**
		 \brief Constructor.
		 \param[in] rect Used only size part of the rect. Incoming rect size can be modified by the UIList if this is neccesary.
		 \param[in] cellIdentifier literal identifier to represents cell type. For example: "Name cell", "Phone number cell", etc.
		 */
    UIListCell(const Rect& rect = Rect(), const String& cellIdentifier = String());

    /**
		 \brief Returns cell's identifier.
		 \returns identifier
		 */
    const String& GetIdentifier() const;

    /**
		 \brief set cell's identifier.
		 \param[in] new cell identifier
		 */
    void SetIdentifier(const String& identifier);
    /**
		 \brief Returns current cell sequence number in the list.
		 \returns list item index
		 */
    int32 GetIndex() const;

    UIListCell* Clone() override;
    void CopyDataFrom(UIControl* srcControl) override;

protected:
    virtual ~UIListCell();

    void SetIndex(int32 index)
    {
        currentIndex = index;
    }

private:
    int32 currentIndex;
    String identifier;

    void* cellStore;
};
}

#endif
#pragma once

#include "Base/BaseTypes.h"

namespace DAVA
{
/**
    Represents an item in `DebugOverlay`.
*/
class DebugOverlayItem
{
public:
    virtual ~DebugOverlayItem(){};

    /** Get string to be used when drawing menu item. */
    virtual String GetName() const = 0;

    /** Draw information. Will be called by `DebugOverlay` every frame if object is enabled. */
    virtual void Draw() = 0;

    /** Called by `DebugOverlay` when item switches to enabled state. */
    virtual void OnShown(){};

    /** Called by `DebugOverlay` when item switches to disabled state. */
    virtual void OnHidden(){};
};
}
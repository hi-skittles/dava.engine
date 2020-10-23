#pragma once

namespace DAVA
{
struct UISingleComponent
{
    virtual ~UISingleComponent() = default;

    /** Restore temporare state of component at the end of each frame. */
    virtual void ResetState() = 0;
};
}
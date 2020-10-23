#pragma once

#include "Functional/Signal.h"
#include "Input/InputEvent.h"
#include "Debug/Private/RingArray.h"

namespace DAVA
{
class Window;

class DebugGestureListener
{
public:
    DebugGestureListener();
    ~DebugGestureListener();
    void AddListenerOnMouseAndTouch();
    Signal<> debugGestureMatch;

private:
    void OnGameLoopStarted();
    bool OnMouseOrTouch(const InputEvent& ev);
    RingArray<InputEvent> history;
    uint32 handlerToken = 0;
};
}

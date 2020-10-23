#pragma once

#include "Animation/AnimatedObject.h"
#include "Animation/Interpolation.h"
#include "Base/BaseTypes.h"
#include "UI/Styles/UIStyleSheetPropertyDataBase.h"
#include "UI/UIGeometricData.h"

namespace DAVA
{
class Animation;
class EventDispatcher;
class Message;
class Sprite;
class UIComponent;
class UIControlBackground;
class UIControlFamily;
class UIControlPackageContext;
class UIControlSystem;
class UIEvent;

#define CONTROL_TOUCH_AREA 15

/**
     \ingroup controlsystem
     \brief Base control system unit.
        Responsible for update, draw and user input processing.

        Methods call sequence:
        When the control adds to the hierarchy:

            -if hierarchy is allready on the screen SystemActive() will be called after adding control to hierarhy. SystemActive()
                calls OnActive() for the control and then calls SystemActive() for all control children.

            -if hierarchy is not on the screen methods would be called only when the hierarchy parent
                be placed to the screen.

        When the control removes from hierarchy:

            -SystemInactive() will be called. SystemInactive()
                calls OnInactive() for the control and then calls SystemInactive() for all control children.

        Every frame:

            -SystemDraw() is calls. SystemDraw() calculates current control geometric data. Transmit information
                about the parent color to the control background. Sets clip if requested. Calls Draw().
                Calls SystemDraw() for the all control children. Calls DrawAfterChilds(). Returns clip back.
                Draw() method proceed control background drawing by default.
                You can't remove, add or sort controls on the draw step.

        Every input:

            -SystemInput() is calls. At the first control process SystemInput() for all their children. If one
                of the children returns true from their SystemInput(), control is returns true too. If no one
                of the children returns true control is returns result of the SystemProcessInput() method.

            -SystemProcessInput() method checks if the control is responsible to process current input. If input
                is possible to process, SystemProcessInput() sets system flags and calls Input() method, then returns true.
                If input is inpossible to process SystemProcessInput() returns false.

        Each control contain UIControlBackground object responsible for visual
        representation. UIControlBackground can be changed for the custom. Or you can
        just overload Draw() method for the custom drawing.
     */
class UIControl : public AnimatedObject
{
    friend class UIInputSystem;
    friend class UIControlSystem;
    friend class UILayoutSystem; // Need for isIteratorCorrupted. See UILayoutSystem::UpdateControl.
    friend class UIRenderSystem; // Need for isIteratorCorrupted. See UILayoutSystem::UpdateControl.
    DAVA_VIRTUAL_REFLECTION(UIControl, AnimatedObject);

public:
    /**
     \enum Control state bits.
     */
    enum eControlState
    {
        STATE_NORMAL = 1 << 0, //!<Control isn't under influence of any activities.
        STATE_PRESSED_OUTSIDE = 1 << 1, //!<Mouse or touch comes into control but dragged outside of control.
        STATE_PRESSED_INSIDE = 1 << 2, //!<Mouse or touch comes into control.
        STATE_DISABLED = 1 << 3, //!<Control is disabled (don't process any input). Use this state only if you want change graphical representation of the control. Don't use this state for the disabling inputs for parts of the controls hierarchy!.
        STATE_SELECTED = 1 << 4, //!<Just a state for base control, nothing more.
        STATE_HOVER = 1 << 5, //!<This bit is rise then mouse is over the control.
        STATE_FOCUSED = 1 << 6, //!<Control under focus and will receive keyboard input. Additional this state can be used for setting visual style of control.

        STATE_COUNT = 7
    };

    static const char* STATE_NAMES[STATE_COUNT];

    /**
     \enum Control events supported by default.
     */
    enum eEventType
    {
        EVENT_TOUCH_DOWN = 1, //!<Trigger when mouse button or touch comes down inside the control.
        EVENT_TOUCH_UP_INSIDE = 2, //!<Trigger when mouse pressure or touch processed by the control is released.
        EVENT_VALUE_CHANGED = 3, //!<Used with sliders, spinners and switches. Trigger when value of the control is changed. Non-NULL callerData means that value is changed from code, not from UI.
        EVENT_HOVERED_SET = 4, //!<
        EVENT_HOVERED_REMOVED = 5, //!<
        EVENT_FOCUS_SET = 6, //!<Trigger when control becomes focused
        EVENT_FOCUS_LOST = 7, //!<Trigger when control lost focus
        EVENT_TOUCH_UP_OUTSIDE = 8, //!<Trigger when mouse pressure or touch processed by the control is released outside of the control.
        EVENTS_COUNT
    };

public:
    /**
     \brief Creates control with requested size and position.
     \param[in] rect Size and coordinates of control you want.
     \param[in] rectInAbsoluteCoordinates Send 'true' if you want to make control in screen coordinates.
        Rect coordinates will be recalculated to the hierarchy coordinates.
        Warning, rectInAbsoluteCoordinates isn't properly works for now!
     */
    UIControl(const Rect& rect = Rect());

    /**
     \brief Returns Sprite frame used for draw in the current UIControlBackground object.
        You can call this function directly for the controlBackgound.
     \returns Sprite frame used for draw.
     */
    DAVA_DEPRECATED(int32 GetFrame() const);

    /**
     \brief Returns untransformed control rect.
        To get control metrics that applies all control transformation you need to use
        geometric data received with GetGeometricData().
     \returns control rect.
     */
    inline Rect GetRect() const;

    /**
     \brief Returns absolute untransformed control rect.
        To get control metrics that applies all control transformation you need to use
        geometric data received with GetGeometricData().
     \returns control rect.
     */
    Rect GetAbsoluteRect() const;

    /**
     \brief Sets the untransformed control rect.
     \param[in] rect new control rect.
     */
    virtual void SetRect(const Rect& rect);

    /**
     \brief Sets the untransformed control absolute rect.
     \param[in] rect new control absolute rect.
     */
    void SetAbsoluteRect(const Rect& rect);

    /**
     \brief Returns untransformed control position.
        To get control metrics that applies all control transformation you need to use
        geometric data received with GetGeometricData().
     \returns control position.
     */
    inline const Vector2& GetPosition() const;

    /**
     \brief Returns untransformed control position.
        To get control metrics that applies all control transformation you need to use
        geometric data received with GetGeometricData().
     \returns control absolute position.
     */
    Vector2 GetAbsolutePosition() const;

    /**
     \brief Sets the untransformed control position.
     \param[in] position new control position.
     */
    virtual void SetPosition(const Vector2& position);

    /**
     \brief Sets the untransformed control absolute position.
     \param[in] position new control absolute position.
     */
    void SetAbsolutePosition(const Vector2& position);

    /**
     \brief Returns untransformed control size.
        To get control metrics that applies all control transformation you need to use
        geometric data received with GetGeometricData().
     \returns control size.
     */
    inline const Vector2& GetSize() const;

    /**
     \brief Sets the untransformed control size.
     \param[in] newSize new control size.
     */
    virtual void SetSize(const Vector2& newSize);

    /**
     \brief Returns control pivot point.
     \returns control pivot point.
     */
    inline Vector2 GetPivotPoint() const;

    /**
     \brief Sets the control pivot point.
     \param[in] newPivot new control pivot point.
     */
    void SetPivotPoint(const Vector2& newPivot);

    /**
     \brief Returns control pivot.
     \returns control pivot.
     */
    inline const Vector2& GetPivot() const;

    /**
     \brief Sets the control pivot.
     \param[in] newPivot new control pivot.
     */
    void SetPivot(const Vector2& newPivot);

    /**
     \brief Returns control scale.
     \returns control scale.
     */
    inline const Vector2& GetScale() const;

    /**
     \brief Sets the control scale.
     \param[in] newScale new control scale.
     */
    inline void SetScale(const Vector2& newScale);

    /**
     \brief Returns actual control transformation and metrics.
     \returns control geometric data.
     */
    virtual const UIGeometricData& GetGeometricData() const;

    /**
     \brief Returns actual control local transformation and metrics.
     \returns control geometric data.
     */
    UIGeometricData GetLocalGeometricData() const;

    /**
     \brief Sets the scaled control rect.
        This method didn't apply any changes to the control size, but recalculate control scale.
     Warning, rectInAbsoluteCoordinates isn't properly works for now!
     \param[in] rect new control rect.
     */
    virtual void SetScaledRect(const Rect& rect, bool rectInAbsoluteCoordinates = false);

    /**
     \brief Returns control rotation angle in radians.
     \returns control angle in radians.
     */
    inline float32 GetAngle() const;
    float32 GetAngleInDegrees() const;

    /**
     \brief Sets control rotation angle in radians.
        Control rotates around the pivot point.
     \param[in] angleInRad new control angle in radians.
     */
    virtual void SetAngle(float32 angleInRad);

    void SetAngleInDegrees(float32 angle);

    /**
     \brief Returns control visibility.
        Invisible controls don't process any inputs.
        Also for invisible controls didn't calls Draw() and DrawAfterChilds() methods.
     \returns control visibility.
     */
    inline bool GetVisibilityFlag() const;

    /**
     \brief Sets control recursive visibility.
        Invisible controls don't process any inputs.
        Also for invisible controls didn't calls Draw() and DrawAfterChilds() methods.
     \param[in] isVisible new control visibility.
     */
    virtual void SetVisibilityFlag(bool isVisible);

    /**
     \brief Returns control input processing ability.
        Be ware! Base control processing inputs by default.
     \returns true if control processing inputs.
     */
    inline bool GetInputEnabled() const;

    /**
     \brief Sets control input processing ability.
        If input is disabled control don't process any inputs. If input is disabled all inputs events would comes to the parent control.
        Please use input enabling/disabling for the single controls or for the small parts of hierarchy.
        It's always better to add transparent control that covers all screen and would process all
        incoming inputs to prevent input processing for the all screen controls or for the large part of hierarchy.
     \param[in] isEnabled is control should process inputs?
     \param[in] hierarchic use true if you want to all control children change input ability.
     */
    virtual void SetInputEnabled(bool isEnabled, bool hierarchic = true);

    /**
     \brief Returns control enabling state.
        Disabled control don't process any inputs. But allows input processing for their children.
        Use this state only if you want change graphical representation of the control.
        Don't use this state for the disabling inputs for parts of the controls hierarchy!
        All controls is enabled by default.
     \returns true if control is disabled.
     */
    bool GetDisabled() const;

    /**
     \brief Sets the control enabling/disabling.
        Disabled control don't process any inputs. But allows input processing for their children.
        Use this state only if you want change graphical representation of the control.
        Don't use this state for the disabling inputs for parts of the controls hierarchy!
        All controls is enabled by default.
     \param[in] isDisabled is control disabled?
     \param[in] hierarchic use true if you want to all control children change enabling/disabling.
     */
    virtual void SetDisabled(bool isDisabled, bool hierarchic = true);

    /**
     \brief Returns control selection state.
     \returns is control selected.
     */
    bool GetSelected() const;

    /**
     \brief Sets control selection state.
        Selection state don't influence on any control activities.
     \param[in] isSelected is control selected?
     \param[in] hierarchic use true if you want to all control children change selection state.
     */
    virtual void SetSelected(bool isSelected, bool hierarchic = true);

    /**
     \brief Returns control hover state.
        Only controls what processed inputs may be hovered.
     \returns control hover state is true if mouse placed over the control rect and no mouce buttons is pressed.
     */
    bool GetHover() const;

    /**
     \brief Is exclusive input enabled.
        If control have exclusive input enabled and this control starts to process
        inputs. All inputs would be directed only to this control. But this control can
        process multiple number of inputs at a time.
        Exclusive input is disabled by default.
     \returns true if control supports exclusive input.
     */
    inline bool GetExclusiveInput() const;
    /**
     \brief Enables or disables control exclusive input.
        If control have exclusive input enabled and this control starts to process
        inputs. All inputs would be directed only to this control. But this control can
        process multiple number of inputs at a time.
        Exclusive input is disabled by default.
     \param[in] isExclusiveInput should control process inputs exclusively?
     \param[in] hierarchic use true if you want to all control children change exclusive input state.
     */
    virtual void SetExclusiveInput(bool isExclusiveInput, bool hierarchic = true);
    /**
     \brief Checks if control is multiple input enabled.
        If multiple input is enabled control can process all incoming inputs (Two or
        more touches for example). Otherwise control process only first incoming input.
        Multiply input is disabled by default.
     \returns true if control supports multiple inputs.
     */
    inline bool GetMultiInput() const;
    /**
     \brief Sets control multi input processing.
        If multiple input is enabled control can process all incoming inputs (Two or
        more touches for example). Otherwise control process only first incoming input.
        Multiply input is disabled by default.
     \param[in] isMultiInput should control supports multiple inputs?
     \param[in] hierarchic use true if you want to all control children change multi input support state.
     */
    virtual void SetMultiInput(bool isMultiInput, bool hierarchic = true);
    /**
    \brief Children will be sorted with predicate.
    Function uses stable sort, sets layout dirty flag and invalidates iteration.
    \param[in] predicate sorting predicate. All predicates for std::list<>::sort are allowed for this function too.
    */
    using SortFunction = Function<bool(const RefPtr<UIControl>&, const RefPtr<UIControl>&)>;
    void SortChildren(const SortFunction& predicate);

    /*
     \brief Sets the control name.
        Later you can find control by this name.
     \param[in] name_ new control name.
     */
    void SetName(const String& name_);
    void SetName(const FastName& name_);

    /**
     \brief Returns current name of the control.
     \returns control name.
     */
    inline const FastName& GetName() const;

    /**
     \brief Sets the control tag.
     \param[in] tag new control tag.
     */
    void SetTag(int32 tag);

    /**
     \brief Returns current control tag.
     \returns control tag.
     */
    inline int32 GetTag() const;

    /**
     \brief Returns control with given name.
     \param[in] name requested control name.
     \param[in] recursive use true if you want fro recursive search.
     \returns first control with given name.
     */
    UIControl* FindByName(const String& name, bool recursive = true) const;
    UIControl* FindByName(const FastName& name, bool recursive = true) const;

    const UIControl* FindByPath(const String& path) const;
    UIControl* FindByPath(const String& path);

    template <class C>
    C FindByPath(const String& path) const
    {
        return DynamicTypeCheck<C>(FindByPath(path));
    }

    template <class C>
    C FindByPath(const String& path)
    {
        return DynamicTypeCheck<C>(FindByPath(path));
    }

    /**
     \brief Returns control state bit mask.
     \returns control state.
     */
    inline int32 GetState() const;
    /**
     \brief Sets control state bit mask.
        Try to not use this method manually.
     \param[in] state new control bit mask.
     */
    void SetState(int32 state);

    UIControlSystem* GetScene() const;

    /**
     \brief Returns control parent.
     \returns if control hasn't parent returns NULL.
     */
    UIControl* GetParent() const;

    /**
     \brief Returns list of control children.
     \returns list of control children.
     */
    const List<RefPtr<UIControl>>& GetChildren() const;
    /**
     \brief Add control as a child.
        Children draws in the sequence of adding. If child has another parent
        this child removes from the parent firstly.
     \param[in] control control to add.
     */
    void AddControl(RefPtr<UIControl> control);
    DAVA_DEPRECATED(virtual void AddControl(UIControl* control));
    /**
     \brief Removes control from the children list.
        If child isn't present in the method owners list nothing happens.
     \param[in] control control to remove.
     */
    void RemoveControl(RefPtr<UIControl> control);
    DAVA_DEPRECATED(virtual void RemoveControl(UIControl* control));
    /**
     \brief Remove this control from its parent, if any.
     */
    virtual void RemoveFromParent();
    /**
     \brief Removes all children from the control.
     */
    virtual void RemoveAllControls();
    /**
     \brief Brings given child front.
        This child will be drawn at the top of the control children.
        If child isn't present in the owners list nothing happens.
     \param[in] _control control to bring front.
     */
    virtual void BringChildFront(const UIControl* _control);
    /**
     \brief Brings given child back.
        This child will be drawn at the bottom of the control children.
        If child isn't present in the owners list nothing happens.
     \param[in] _control control to bring back.
     */
    virtual void BringChildBack(const UIControl* _control);
    /**
     \brief Inserts given child before the requested.
     \param[in] _control control to insert.
     \param[in] _belowThisChild control to insert before. If this control isn't present in the
        children list new child adds at the top of the list.
     */
    void InsertChildBelow(RefPtr<UIControl> _control, const UIControl* _belowThisChild);
    DAVA_DEPRECATED(virtual void InsertChildBelow(UIControl* _control, const UIControl* _belowThisChild));
    /**
     \brief Inserts given child after the requested.
     \param[in] _control control to insert.
     \param[in] _aboveThisChild control to insert after. If this control isn't present in the
     children list new child adds at the top of the list.
     */
    void InsertChildAbove(RefPtr<UIControl> _control, const UIControl* _aboveThisChild);
    DAVA_DEPRECATED(virtual void InsertChildAbove(UIControl* _control, const UIControl* _aboveThisChild));
    /**
     \brief Sends given child before the requested.
        If one of the given children isn't present in the owners list nothing happens.
     \param[in] _control control to move.
     \param[in] _belowThisChild control to sends before.
     */
    virtual void SendChildBelow(const UIControl* _control, const UIControl* _belowThisChild);
    /**
     \brief Sends given child after the requested.
        If one of the given children isn't present in the owners list nothing happens.
     \param[in] _control control to move.
     \param[in] _aboveThisChild control to sends after.
     */
    virtual void SendChildAbove(const UIControl* _control, const UIControl* _aboveThisChild);

    /**
     \brief Adds callback message for the event trigger.
     \param[in] eventType event type you want to process.
     \param[in] msg message should be called when the event triggered.
     */
    void AddEvent(int32 eventType, const Message& msg);
    /**
     \brief Removes callback message for the event trigger.
     \param[in] eventType event type you want to remove.
     \param[in] msg message to remove.
     \returns true if event is removed.
     */
    bool RemoveEvent(int32 eventType, const Message& msg);
    /**
     \brief Function to remove all events from event dispatcher.
     \returns true if we removed something, false if not
     */
    bool RemoveAllEvents();

    /**
     \brief Send given event to the all subscribed objects.
     \param[in] eventType event type you want to process.
     \param[in] uiEvent input event that triggered this control event.
     */
    void PerformEvent(int32 eventType, const UIEvent* uiEvent = nullptr);
    /**
     \brief Send given event with given user data to the all subscribed objects.
     \param[in] eventType event type you want to process.
     \param[in] callerData data you want to send to the all messages.
     \param[in] uiEvent input event that triggered this control event.
     */
    void PerformEventWithData(int32 eventType, void* callerData, const UIEvent* uiEvent = nullptr);

    /**
     \brief Creates the absolutely identical copy of the control.
     \returns control copy.
     */
    virtual UIControl* Clone();

    RefPtr<UIControl> SafeClone();
    /**
     \brief Copies all control parameters from the sent control.
     \param[in] srcControl Source control to copy parameters from.
     */
    virtual void CopyDataFrom(UIControl* srcControl);

    //Animation helpers

    /**
     \brief Starts wait animation for the control.
     \param[in] time animation time.
     \param[in] track animation track. 0 by default.
     \returns Animation object
     */
    Animation* WaitAnimation(float32 time, int32 track = 0);
    /**
     \brief Starts move and size animation for the control.
     \param[in] rect New control position and size.
     \param[in] time animation time.
     \param[in] interpolationFunc time interpolation method.
     \param[in] track animation track. 0 by default.
     \returns Animation object
     */
    Animation* MoveAnimation(const Rect& rect, float32 time, Interpolation::FuncType interpolationFunc = Interpolation::LINEAR, int32 track = 0);
    /**
     \brief Starts move and scale animation for the control. Changing scale instead of size.
     \param[in] rect New control position and size.
     \param[in] time animation time.
     \param[in] interpolationFunc time interpolation method.
     \param[in] track animation track. 0 by default.
     \returns Animation object
     */
    Animation* ScaledRectAnimation(const Rect& rect, float32 time, Interpolation::FuncType interpolationFunc = Interpolation::LINEAR, int32 track = 0);
    /**
     \brief Starts scale animation for the control. Changing scale instead of size.
     \param[in] newSize New control size.
     \param[in] time animation time.
     \param[in] interpolationFunc time interpolation method.
     \param[in] track animation track. 0 by default.
     \returns Animation object
     */
    Animation* ScaledSizeAnimation(const Vector2& newSize, float32 time, Interpolation::FuncType interpolationFunc = Interpolation::LINEAR, int32 track = 0);
    /**
     \brief Starts control position animation.
     \param[in] _position New control position.
     \param[in] time animation time.
     \param[in] interpolationFunc time interpolation method.
     \param[in] track animation track. 0 by default.
     \returns Animation object
     */
    Animation* PositionAnimation(const Vector2& _position, float32 time, Interpolation::FuncType interpolationFunc = Interpolation::LINEAR, int32 track = 0);
    /**
     \brief Starts control size animation.
     \param[in] _size New control size.
     \param[in] time animation time.
     \param[in] interpolationFunc time interpolation method.
     \param[in] track animation track. 0 by default.
     \returns Animation object
     */
    Animation* SizeAnimation(const Vector2& _size, float32 time, Interpolation::FuncType interpolationFunc = Interpolation::LINEAR, int32 track = 0);
    /**
     \brief Starts control scale animation.
     \param[in] newScale New control scale.
     \param[in] time animation time.
     \param[in] interpolationFunc time interpolation method.
     \param[in] track animation track. 0 by default.
     \returns Animation object
     */
    Animation* ScaleAnimation(const Vector2& newScale, float32 time, Interpolation::FuncType interpolationFunc = Interpolation::LINEAR, int32 track = 0);
    /**
     \brief Starts control rotation angle animation.
     \param[in] newAngle New control rotation angle.
     \param[in] time animation time.
     \param[in] interpolationFunc time interpolation method.
     \param[in] track animation track. 0 by default.
     \returns Animation object
     */
    Animation* AngleAnimation(float32 newAngle, float32 time, Interpolation::FuncType interpolationFunc = Interpolation::LINEAR, int32 track = 0);
    /**
     \brief Starts input enabling switching animation. This animation changing control
        input enabling state on the next frame after the animation start.
     \param[in] touchable New input enabled value.
     \param[in] hierarhic Is value need to be changed in all control children.
     \param[in] track animation track. 0 by default.
     \returns Animation object
     */
    Animation* TouchableAnimation(bool touchable, bool hierarhic = true, int32 track = 0);
    /**
     \brief Starts control disabling animation. This animation changing control
        disable state on the next frame after the animation start.
     \param[in] disabled New control disabling value.
     \param[in] hierarhic Is value need to be changed in all control children.
     \param[in] track animation track. 0 by default.
     \returns Animation object
     */
    Animation* DisabledAnimation(bool disabled, bool hierarhic = true, int32 track = 0);
    /**
     \brief Starts control visible animation. This animation changing control visibility
        on the next frame after the animation start.
     \param[in] visible New control recursive visible value.
     \param[in] track animation track. 0 by default.
     \returns Animation object
     */
    Animation* VisibleAnimation(bool visible, int32 track = 0);
    /**
     \brief Starts control remove animation. This animation removes control from the parent
     on the next frame  after the animation start.
     \param[in] track animation track. 0 by default.
     \returns Animation object
     */
    Animation* RemoveControlAnimation(int32 track = 0);
    /**
     \brief Starts control color animation.
     \param[in] New control color.
     \param[in] animation time.
     \param[in] time interpolation method.
     \param[in] track animation track. 0 by default.
     \returns Animation object
     */
    Animation* ColorAnimation(const Color& finalColor, float32 time, Interpolation::FuncType interpolationFunc = Interpolation::LINEAR, int32 track = 0);

protected:
    void TouchableAnimationCallback(BaseObject* caller, void* param, void* callerData);
    void DisabledAnimationCallback(BaseObject* caller, void* param, void* callerData);
    void VisibleAnimationCallback(BaseObject* caller, void* param, void* callerData);
    void RemoveControlAnimationCallback(BaseObject* caller, void* param, void* callerData);

public:
    bool IsHiddenForDebug() const;
    void SetHiddenForDebug(bool hidden);

    /**
     \brief set parent draw color into control
     \param[in] parentColor draw color of parent background.
     */
    virtual void SetParentColor(const Color& parentColor);

    /**
     \brief Calls on every input event. Calls SystemInput() for all control children.
        If no one of the children is processed input. Calls ProcessInput() for the current control.
        Internal method used by ControlSystem.
     \param[in] currentInput Input information.
     \returns true if control processed this input.
     */
    virtual bool SystemInput(UIEvent* currentInput);
    /**
     \brief Process incoming input and if it's necessary calls Input() method for the control.
        Internal method used by ControlSystem.
     \param[in] currentInput Input information.
     \returns true if control processed this input.
     */
    virtual bool SystemProcessInput(UIEvent* currentInput); // Internal method used by ControlSystem

    Function<bool(UIControl*, UIEvent*)> customSystemProcessInput;

    /**
     \brief Calls when input processed by control is canceled.
        Internal method used by ControlSystem.
     \param[in] currentInput Input information.
     */
    virtual void SystemInputCancelled(UIEvent* currentInput);

    /**
     \brief Called when control is set as the hovered (by the mouse) control.
     Internal method used by ControlSystem. Can be overridden only by the people ho knows UI architecture.
     */
    virtual void SystemDidSetHovered();
    /**
     \brief Called when control is not a hovered (by the mouse) control.
     Internal method used by ControlSystem. Can be overridden only by the people ho knows UI architecture.
     */
    virtual void SystemDidRemoveHovered();

    /**
     \brief Called when control is set as the hovered (by the mouse) control.
     Can be overridden to implement start hovering reaction.
     */
    virtual void DidSetHovered();
    /**
     \brief Called when control is not a hovered (by the mouse) control.
     Can be overridden to implement end hovering reaction.
     */
    virtual void DidRemoveHovered();

    /**
     \brief Calls on every input event coming to control.
        Should be overridden to implement custom input reaction.
        During one input processing step into control may come more then one input event.
        For example: Pressing began event and pressing ended or five continuous mouse move events etc.
        Called only if control inputEnable is true.
     \param[in] currentInput Input information.
     */
    virtual void Input(UIEvent* currentInput);
    /**
     \brief Calls when input processed by control is canceled.
        Should be overridden to implement custom input cancelling reaction.
     \param[in] currentInput Input information.
     */
    virtual void InputCancelled(UIEvent* currentInput);
    /**
	 \brief Calls on every frame with frame delata time parameter.
            Works only with added UIUpdateComponent!
            Should be overriden to implement perframe functionality.
            Default realization is empty.
	 \param[in] timeElapsed Current frame time delta.
	 */
    virtual void Update(float32 timeElapsed);
    /**
     \brief Calls on every frame to draw control.
        Can be overridden to implement custom draw functionality.
        Default realization is drawing UIControlBackground with requested parameters.
     \param[in] geometricData Control geometric data.
     */
    virtual void Draw(const UIGeometricData& geometricData);
    /**
     \brief Calls on every frame with UIGeometricData after all children is are drew.
        Can be overridden to implement after children drawing.
        Default realization is empty.
     \param[in] geometricData Control geometric data.
     */
    virtual void DrawAfterChilds(const UIGeometricData& geometricData);

protected:
    enum class eViewState : int32
    {
        INACTIVE,
        ACTIVE,
        VISIBLE,
    };

    virtual void SystemVisible();
    virtual void SystemInvisible();

    virtual void OnVisible();
    virtual void OnInvisible();

    virtual void SystemActive();
    virtual void SystemInactive();

    virtual void OnActive();
    virtual void OnInactive();

    virtual void SystemScreenSizeChanged(const Rect& newFullScreenRect);
    virtual void OnScreenSizeChanged(const Rect& newFullScreenRect);

    void InvokeActive(eViewState parentViewState);
    void InvokeInactive();

    void InvokeVisible(eViewState parentViewState);
    void InvokeInvisible();

    void ChangeViewState(eViewState newViewState);

    void AddState(int32 state);
    void RemoveState(int32 state);

public:
    /**
     \brief Called when this control and his children are loaded.
     */
    virtual void LoadFromYamlNodeCompleted(){};

    /**
     \brief Returns control in hierarchy status.
     \returns True if control in view hierarchy for now.
     */
    bool IsActive() const;

    /**
     \brief Returns control on screen status.
     \returns True if control visible now.
     */
    bool IsVisible() const;
    /**
     \brief Returns point status relative to control.
     \param[in] point Point to check.
     \param[in] expandWithFocus Is area should be expanded with focus.
     \returns True if inside the control rect.
     */
    virtual bool IsPointInside(const Vector2& point, bool expandWithFocus = false) const;

    virtual void SystemOnFocusLost();

    virtual void SystemOnFocused();

    virtual void OnFocusLost();

    virtual void OnFocused();

    virtual void OnTouchOutsideFocus();

    /// sets rect to match background sprite, also moves pivot point to center
    void SetSizeFromBg(bool pivotToCenter = true);

    virtual void UpdateLayout();
    virtual void OnSizeChanged();

    void DumpInputs(int32 depthLevel);

    static void DumpControls(bool onlyOrphans);

private:
    FastName name;
    Vector2 pivot = Vector2(0.0f, 0.0f); //!<control pivot. Top left control corner by default.

    UIControlSystem* scene = nullptr;

    UIControl* parent = nullptr;
    List<RefPtr<UIControl>> children;

    DAVA_DEPRECATED(bool isUpdated = false);
    // Need for old implementation of SystemUpdate.
    friend class UIUpdateSystem;

public:
    //TODO: store geometric data in UIGeometricData
    Vector2 relativePosition; //!<position in the parent control.
    Vector2 size; //!<control size.

    Vector2 scale = Vector2(1.0f, 1.0f); //!<control scale. Scale relative to pivot point.
    float32 angle = 0.0f; //!<control rotation angle. Rotation around pivot point.

protected:
    float32 wheelSensitivity = 30.f;

    // boolean flags are grouped here to pack them together (see please DF-2149).
    bool exclusiveInput : 1;
    bool isInputProcessed : 1;
    bool visible : 1;
    bool hiddenForDebug : 1;
    bool multiInput : 1;

    bool isIteratorCorrupted : 1;

    bool styleSheetDirty : 1;
    bool styleSheetInitialized : 1;
    bool layoutDirty : 1;
    bool layoutPositionDirty : 1;
    bool layoutOrderDirty : 1;

    int32 inputProcessorsCount = 1;

    int32 currentInputID = 0;
    int32 touchesInside = 0;
    int32 totalTouches = 0;

    mutable UIGeometricData tempGeometricData;

    RefPtr<EventDispatcher> eventDispatcher;

    void SetScene(UIControlSystem* scene);
    void SetParent(UIControl* newParent);

    virtual ~UIControl();

    void RegisterInputProcessor();
    void RegisterInputProcessors(int32 processorsCount);
    void UnregisterInputProcessor();
    void UnregisterInputProcessors(int32 processorsCount);

private:
    int32 tag = 0;
    eViewState viewState = eViewState::INACTIVE;
    int32 controlState = eControlState::STATE_NORMAL;

    bool inputEnabled : 1;

public:
    //@{
    /** @name Components */
    /**
    Add specified 'component'. 
    The behavior is undefined unless 'component' is a valid pointer and 'component' is not already added to any UIControl.
    */
    void AddComponent(UIComponent* component);

    /**
    Add specified 'component' at specified 'index' for multi-component support.
    For example, control has 4 UIActionComponents [a, b, c, d] with indeces [0, 1, 2, 3]. If we call InsertComponentAt(e, 2), we will get control with [a, b, e, c, d] components. If we call InsertComponentAt(e, 4 or more), we will get control with [a, b, c, d, e] components. 
    The behavior is undefined unless 'component' is a valid pointer and 'component' is not already added to any UIControl.
    */
    void InsertComponentAt(UIComponent* component, uint32 index);

    /** 
    Remove specified 'component' if it is already added to control.
    The behavior is undefined unless 'component' is a valid pointer.
    */
    void RemoveComponent(UIComponent* component);

    /** Remove component with specified 'type' at specified 'index'. */
    void RemoveComponent(const Type* type, uint32 index = 0);

    /** Remove component with specified 'runtimeType' at specified 'index'. */
    void RemoveComponent(int32 runtimeType, uint32 index = 0);

    /** Remove UIComponent with specified type 'T' at specified 'index'. */
    template <class T>
    void RemoveComponent(int32 index = 0)
    {
        static int32 runtimeType = TypeToRuntimeType(Type::Instance<T>());
        RemoveComponent(runtimeType, index);
    }

    /** Remove all components. */
    void RemoveAllComponents();

    /** Return UIComponent with specified 'type' at specified 'index'. Return nullptr if such component is not found. */
    UIComponent* GetComponent(const Type* type, uint32 index = 0) const;

    /** Return UIComponent with specified 'runtimeType' at specified 'index'. Return nullptr if such component is not found. */
    UIComponent* GetComponent(int32 runtimeType, uint32 index = 0) const;

    /** Return UIComponent with specified type 'T' at specified 'index'. */
    template <class T>
    inline T* GetComponent(uint32 index = 0) const
    {
        static int32 runtimeType = TypeToRuntimeType(Type::Instance<T>());
        return DynamicTypeCheck<T*>(GetComponent(runtimeType, index));
    }

    /** Return UIComponent with specified 'typeName' (reflection permament name) at specified 'index'.
        Return nullptr if such component is not found. */
    UIComponent* GetComponentByName(const String& typeName, uint32 index = 0) const;

    /**
    Return index in UIControl::components of specified 'component'. Return -1 if 'component' is not found in control.
    The behavior is undefined unless 'component' is a valid pointer.
    */
    int32 GetComponentIndex(const UIComponent* component) const;

    /** Return UIComponent with specified 'type' at specified 'index'. 
    If such component is not found, new component with 'type' is created, added to control and returned. 
    In case of creation, the behavior is undefined until 'index' is 0. */
    UIComponent* GetOrCreateComponent(const Type* type, uint32 index = 0);

    /** Return UIComponent with specified type'T' at specified 'index'.
    If such component is not found, new component with 'type' is created, added to control and returned.
    In case of creation, the behavior is undefined until 'index' is 0. */
    template <class T>
    inline T* GetOrCreateComponent(uint32 index = 0)
    {
        return DynamicTypeCheck<T*>(GetOrCreateComponent(Type::Instance<T>(), index));
    }

    /** Return UIComponent with specified 'typeName' at specified 'index'.
    If such component is not found, new component with 'typeName' is created, added to control and returned.
    In case of creation, the behavior is undefined until 'index' is 0. */
    UIComponent* GetOrCreateComponentByName(const String& typeName, uint32 index = 0);

    /** Return total number of components. */
    uint32 GetComponentCount() const;

    /** Return total number of components with specified 'type'. */
    uint32 GetComponentCount(const Type* type) const;

    /** Return total number of components with specified 'runtimeType'. */
    uint32 GetComponentCount(int32 runtimeType) const;

    template <class T>
    inline uint32 GetComponentCount() const
    {
        static int32 runtimeType = TypeToRuntimeType(Type::Instance<T>());
        return GetComponentCount(runtimeType);
    }

    /** Return UIControl::components reference. */
    const Vector<UIComponent*>& GetComponents();
    //@}

private:
    Vector<UIComponent*> components;
    UIControlFamily* family = nullptr;
    void RemoveComponent(const Vector<UIComponent*>::iterator& it);
    void UpdateFamily();

    static int32 TypeToRuntimeType(const Type* type);

    /* Styles */
public:
    void AddClass(const FastName& clazz);
    void RemoveClass(const FastName& clazz);
    bool HasClass(const FastName& clazz) const;
    void SetTaggedClass(const FastName& tag, const FastName& clazz);
    FastName GetTaggedClass(const FastName& tag) const;
    void ResetTaggedClass(const FastName& tag);

    String GetClassesAsString() const;
    void SetClassesFromString(const String& classes);

    const UIStyleSheetPropertySet& GetLocalPropertySet() const;
    void SetLocalPropertySet(const UIStyleSheetPropertySet& set);
    void SetPropertyLocalFlag(uint32 propertyIndex, bool value);

    const UIStyleSheetPropertySet& GetStyledPropertySet() const;
    void SetStyledPropertySet(const UIStyleSheetPropertySet& set);

    bool IsStyleSheetInitialized() const;
    void SetStyleSheetInitialized();

    bool IsStyleSheetDirty() const;
    void SetStyleSheetDirty();
    void ResetStyleSheetDirty();

    bool IsLayoutDirty() const;
    void SetLayoutDirty();
    void ResetLayoutDirty();

    bool IsLayoutPositionDirty() const;
    void SetLayoutPositionDirty();
    void ResetLayoutPositionDirty();

    bool IsLayoutOrderDirty() const;
    void SetLayoutOrderDirty();
    void ResetLayoutOrderDirty();

    RefPtr<UIControlPackageContext> GetPackageContext() const;
    const RefPtr<UIControlPackageContext>& GetLocalPackageContext() const;
    void SetPackageContext(const RefPtr<UIControlPackageContext>& packageContext);
    UIControl* GetParentWithContext() const;

private:
    UIStyleSheetClassSet classes;
    UIStyleSheetPropertySet localProperties;
    UIStyleSheetPropertySet styledProperties;
    RefPtr<UIControlPackageContext> packageContext;
    UIControl* parentWithContext = nullptr;

    void PropagateParentWithContext(UIControl* newParentWithContext);
    /* Styles */

public:
    inline float32 GetWheelSensitivity() const;
    inline void SetWheelSensitivity(float32 newSens);

    // for introspection
    inline bool GetEnabled() const;
    inline void SetEnabledNotHierarchic(bool enabled);
    inline void SetSelectedNotHierarchic(bool enabled);
    inline void SetExclusiveInputNotHierarchic(bool enabled);
    inline bool GetNoInput() const;
    inline void SetNoInput(bool noInput);
};

inline Vector2 UIControl::GetPivotPoint() const
{
    return pivot * size;
}

inline const Vector2& UIControl::GetPivot() const
{
    return pivot;
}

inline const Vector2& UIControl::GetScale() const
{
    return scale;
}

inline void UIControl::SetScale(const Vector2& newScale)
{
    scale = newScale;
}

inline const Vector2& UIControl::GetSize() const
{
    return size;
}

inline const Vector2& UIControl::GetPosition() const
{
    return relativePosition;
}

inline float32 UIControl::GetAngle() const
{
    return angle;
}

inline const FastName& UIControl::GetName() const
{
    return name;
}

inline int32 UIControl::GetTag() const
{
    return tag;
}

inline Rect UIControl::GetRect() const
{
    return Rect(GetPosition() - GetPivotPoint(), GetSize());
}

inline bool UIControl::GetVisibilityFlag() const
{
    return visible;
}

inline bool UIControl::GetInputEnabled() const
{
    return inputEnabled;
}

inline bool UIControl::GetExclusiveInput() const
{
    return exclusiveInput;
}

inline bool UIControl::GetMultiInput() const
{
    return multiInput;
}

inline int32 UIControl::GetState() const
{
    return controlState;
}

inline bool UIControl::GetEnabled() const
{
    return !GetDisabled();
}

inline void UIControl::SetEnabledNotHierarchic(bool enabled)
{
    SetDisabled(!enabled, false);
}

inline void UIControl::SetSelectedNotHierarchic(bool selected)
{
    SetSelected(selected, false);
}

inline void UIControl::SetExclusiveInputNotHierarchic(bool enabled)
{
    SetExclusiveInput(enabled, false);
}

inline bool UIControl::GetNoInput() const
{
    return !GetInputEnabled();
}

inline void UIControl::SetNoInput(bool noInput)
{
    SetInputEnabled(!noInput, false);
}

inline float32 UIControl::GetWheelSensitivity() const
{
    return wheelSensitivity;
}
inline void UIControl::SetWheelSensitivity(float32 newSens)
{
    wheelSensitivity = newSens;
}

inline bool UIControl::IsLayoutDirty() const
{
    return layoutDirty;
}

inline bool UIControl::IsLayoutPositionDirty() const
{
    return layoutPositionDirty;
}

inline bool UIControl::IsLayoutOrderDirty() const
{
    return layoutOrderDirty;
}
};

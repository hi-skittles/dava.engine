#pragma once

#include "Base/BaseTypes.h"
#include "Render/2D/TextBlock.h"
#include "UI/UIControl.h"
#include "UI/UIControlBackground.h"

namespace DAVA
{
/**
     \ingroup controlsystem
     \brief Button.
        Use for different graphical representation of the control state.
        Every eControlState can be represented by the different UIControlBackground and different text.
        Only UIControlBackground for the STATE_NORMAL is present by default. No text for states is present by default.
        You should call one of the setter methods to create a UIControlBackground or a text control for a state.
        UIButton presents some accessors for the UITextControl and UIControlBackground, but for the full functionality
        you should use GetStateBackground() and GetStateTextControl().
     */
class UIStaticText;
class Font;

class UIButton : public UIControl
{
    DAVA_VIRTUAL_REFLECTION(UIButton, UIControl);

public:
    /**
     \brief Creates button with requested size and position.
     \param[in] rect Size and coordinates of control you want.
     \param[in] rectInAbsoluteCoordinates Send 'true' if you want to make control in screen coordinates.
        Rect coordinates will be recalculated to the hierarchy coordinates.
        Warning, rectInAbsoluteCoordinates isn't properly works for now!
     */
    UIButton(const Rect& rect = Rect());

    void SetRect(const Rect& rect) override;

    void SetSize(const Vector2& newSize) override;

    /**
     \brief Returns Sprite used for draw requested state in the UIControlBackground object.
     \param[in] state state to get value for.
     \returns Sprite used for draw.
     */
    virtual Sprite* GetStateSprite(int32 state);
    /**
     \brief Returns Sprite frame used for draw requested state in the UIControlBackground object.
     \param[in] state state to get value for.
     \returns Sprite frame used for draw.
     */
    virtual int32 GetStateFrame(int32 state);
    /**
     \brief Returns draw type used for draw requested state in the UIControlBackground object.
     \param[in] state state to get value for.
     \returns Draw type used for draw.
     */
    virtual UIControlBackground::eDrawType GetStateDrawType(int32 state);
    /**
     \brief Returns Sprite align used for draw requested state in the UIControlBackground object.
     \param[in] state state to get value for.
     \returns Sprite frame used for draw.
     */
    virtual int32 GetStateAlign(int32 state);
    /**
     \brief Sets Sprite for the control UIControlBackground object for the requested state.
        Method creates UIControlBackground object for the state if this is neccesary.
     \param[in] state state bit mask to set value for.
     \param[in] spriteName Sprite path-name.
     \param[in] spriteFrame Sprite frame you want to use for draw.
     */
    virtual void SetStateSprite(int32 state, const FilePath& spriteName, int32 spriteFrame = 0);
    /**
     \brief Sets Sprite for the control UIControlBackground object for the requested state.
        Method creates UIControlBackground object for the state if this is neccesary.
     \param[in] state state bit mask to set value for.
     \param[in] newSprite Pointer for a Sprite.
     \param[in] spriteFrame Sprite frame you want to use for draw.
     */
    virtual void SetStateSprite(int32 state, Sprite* newSprite, int32 spriteFrame = 0);
    /**
     \brief Sets Sprite frame you want to use for draw for the control UIControlBackground object for the requested state.
        Method creates UIControlBackground object for the state if this is neccesary.
     \param[in] state state bit mask to set value for.
     \param[in] spriteFrame Sprite frame.
     */
    virtual void SetStateFrame(int32 state, int32 spriteFrame);
    /**
     \brief Sets draw type you want to use the control UIControlBackground object for the requested state.
        Method creates UIControlBackground object for the state if this is neccesary.
     \param[in] state state bit mask to set value for.
     \param[in] drawType Draw type to use for drawing.
     */
    virtual void SetStateDrawType(int32 state, UIControlBackground::eDrawType drawType);
    /**
     \brief Sets Sprite align you want to use for draw for the control UIControlBackground object for the requested state.
        Method creates UIControlBackground object for the state if this is neccesary.
     \param[in] state state bit mask to set value for.
     \param[in] align Sprite align.
     */
    virtual void SetStateAlign(int32 state, int32 align);
    /**
     \brief Sets Sprite reflection you want to use for draw for the control UIControlBackground object for the requested state.
        Method creates UIControlBackground object for the state if this is neccesary.
     \param[in] state state bit mask to set value for.
     \param[in] modification Sprite modification(reflection).
     */
    virtual void SetStateModification(int32 state, int32 modification);
    /**
     \brief Sets Sprite color you want to use for draw for the control UIControlBackground object for the requested state.
        Method creates UIControlBackground object for the state if this is neccesary.
     \param[in] state state bit mask to set value for.
     \param[in] color Sprite color.
     */
    virtual void SetStateColor(int32 state, Color color);
    /**
     \brief Sets Sprite's color inheritence type you want to use for draw for the control UIControlBackground object for the requested state.
        Method creates UIControlBackground object for the state if this is neccesary.
     \param[in] state state bit mask to set value for.
     \param[in] value type of color inheritence.
     */
    virtual void SetStateColorInheritType(int32 state, UIControlBackground::eColorInheritType value);
    /**
     \brief Sets Sprite's per pixel accuracy type you want to use for draw for the control UIControlBackground object for the requested state.
        Method creates UIControlBackground object for the state if this is neccesary.
     \param[in] state state bit mask to set value for.
     \param[in] value type of pixel accuracy.
     */
    virtual void SetStatePerPixelAccuracyType(int32 state, UIControlBackground::ePerPixelAccuracyType value);
    /**
     \brief Returns background used for drawing of the requested state.
     \param[in] state state to get value for.
     \returns background used for state draw.
     */
    virtual UIControlBackground* GetStateBackground(int32 state);
    /**
     \brief Sets background what will be used for draw of the requested states.
        Background is cloned inside button.
     \param[in] state state bit mask to set value for.
     \param[in] newBackground control background you want to use for draw of the states.
     */
    virtual void SetStateBackground(int32 state, UIControlBackground* newBackground);
    /**
     \brief Sets the fitting option for the text background of the requested states.
     \param[in] state state bit mask to set value for.
     \param[in] Fitting option.
     */
    virtual void SetStateFittingOption(int32 state, int32 fittingOption);

    /**
     \brief Sets background what will be used for draw of the requested states.
        Method creates UIStaticText control for the state if this is neccesary.
     \param[in] state state bit mask to set value for.
     \param[in] font font used for text draw of the states.
     */
    virtual void SetStateFont(int32 state, Font* font);

    /**
     \brief Sets font size what will be used for draw of the requested states.
        Method creates UIStaticText control for the state if this is neccesary.
     \param[in] state state bit mask to set value for.
     \param[in] size font's size used for text draw of the states.
     */
    virtual void SetStateFontSize(int32 state, float32 size);

    /**
     \brief Sets the color of the font for particular state.
     \param[in] state state bit mask to set value for.
     \param[in] color font used for text draw of the states.
     */
    virtual void SetStateFontColor(int32 state, const Color& fontColor);

    /**
     \brief Sets the color inherit type of the text and shadow for particular state.
     \param[in] state state bit mask to set value for.
     \param[in] color font used for text draw of the states.
     */
    void SetStateTextColorInheritType(int32 state, UIControlBackground::eColorInheritType colorInheritType);

    /**
     \brief Sets the per pixel accuracy type of the text and shadow for particular state.
     \param[in] state state bit mask to set value for.
     \param[in] color font used for text draw of the states.
     */
    void SetStateTextPerPixelAccuracyType(int32 state, UIControlBackground::ePerPixelAccuracyType pixelAccuracyType);

    /**
     \brief Sets the color of the shadow for particular state.
     \param[in] state state bit mask to set value for.
     \param[in] color font used for text draw of the states.
     */
    virtual void SetStateShadowColor(int32 state, const Color& shadowColor);

    /**
     \brief Sets the offset of the shadow for particular state.
     \param[in] state state bit mask to set value for.
     \param[in] color font used for text draw of the states.
     */
    virtual void SetStateShadowOffset(int32 state, const Vector2& offset);

    /**
     \brief Returns text control used for the requested state.
     \param[in] state state to get value for.
     \returns UIStaticText used for a state.
     */
    virtual UIStaticText* GetStateTextControl(int32 state);

    /**
     \brief Sets text what will be shown for the requested states.
     UIStaticText is cloned inside button.
     \param[in] state state bit mask to set value for.
     \param[in] text text you want to be shown for the text of the states.
     \param[in] requestedTextRectSize rect size to fit text in.
     */
    virtual void SetStateText(int32 state, const WideString& text, const Vector2& requestedTextRectSize = Vector2(0, 0));
    /**
     \brief Sets text align what will be shown for the requested states.
     \param[in] state state text bit mask to set value for.
     \param[in] align the align .
     */
    virtual void SetStateTextAlign(int32 state, int32 align);
    /**
     \brief Sets text use RTL align flag what will be shown for the requested states.
     \param[in] state state text bit mask to set value for.
     \param[in] value using RTL align flag.
     */
    virtual void SetStateTextUseRtlAlign(int32 state, TextBlock::eUseRtlAlign value);
    /**
    \brief Sets text multiline what will be shown for the requested states.
    \param[in] state state text bit mask to set value for.
    \param[in] value multiline by symbols flag.
    */
    virtual void SetStateTextMultiline(int32 state, bool value);
    /**
    \brief Sets text multiline by symbols what will be shown for the requested states.
    \param[in] state state text bit mask to set value for.
    \param[in] value multiline by symbols flag.
    */
    virtual void SetStateTextMultilineBySymbol(int32 state, bool value);
    /**
     \brief Sets text control what will be used for the requested states.
        UIStaticText is cloned inside button.
     \param[in] state state bit mask to set value for.
     \param[in] textControl control you want to use for the text of the states.
     */
    virtual void SetStateTextControl(int32 state, UIStaticText* textControl);

    void Input(UIEvent* currentInput) override;

    void Update(float32 timeElapsed) override;

    void SetParentColor(const Color& parentColor) override;
    UIButton* Clone() override;
    void CopyDataFrom(UIControl* srcControl) override;

    /**
     \brief Creates the background for the UIButton for particular state and caches it.
     Will create the background once only and then cache it.
     */
    virtual void CreateBackgroundForState(int32 state);

    UIControlBackground* GetBackground() const;
    void SetBackground(UIControlBackground* newBg);

protected:
    virtual ~UIButton();

public:
    enum eButtonDrawState
    {
        DRAW_STATE_UNPRESSED = 0,
        DRAW_STATE_PRESSED_OUTSIDE,
        DRAW_STATE_PRESSED_INSIDE,
        DRAW_STATE_DISABLED,
        DRAW_STATE_SELECTED,
        DRAW_STATE_HOVERED

        ,
        DRAW_STATE_COUNT
    };
    UIControlBackground* stateBacks[DRAW_STATE_COUNT];
    UIStaticText* stateTexts[DRAW_STATE_COUNT];

    UIStaticText* selectedTextBlock;

    int32 oldControlState;

    static eButtonDrawState ControlStateToDrawState(int32 state);
    static eControlState DrawStateToControlState(eButtonDrawState state);
    static const String& DrawStatePostfix(eButtonDrawState state);
    static eButtonDrawState GetStateReplacer(eButtonDrawState drawState);

private:
    eButtonDrawState GetActualBackgroundState(eButtonDrawState drawState) const;
    UIControlBackground* GetActualBackgroundForState(int32 state) const;
    UIControlBackground* GetBackground(eButtonDrawState drawState) const
    {
        return stateBacks[drawState];
    }
    UIControlBackground* GetActualBackground(eButtonDrawState drawState) const
    {
        return stateBacks[GetActualBackgroundState(drawState)];
    }
    UIControlBackground* GetOrCreateBackground(eButtonDrawState drawState);
    void SetBackground(eButtonDrawState drawState, UIControlBackground* newBackground);
    UIControlBackground* CreateDefaultBackground() const;

    eButtonDrawState GetActualTextBlockState(eButtonDrawState drawState) const;
    UIStaticText* GetActualTextBlockForState(int32 state) const;
    UIStaticText* GetTextBlock(eButtonDrawState drawState) const
    {
        return stateTexts[drawState];
    }
    UIStaticText* GetActualTextBlock(eButtonDrawState drawState) const
    {
        return stateTexts[GetActualTextBlockState(drawState)];
    }
    UIStaticText* GetOrCreateTextBlock(eButtonDrawState drawState);
    void SetTextBlock(eButtonDrawState drawState, UIStaticText* newTextBlock);
    UIStaticText* CreateDefaultTextBlock() const;

    void UpdateStateTextControlSize();

    void UpdateSelectedTextBlock();

public:
};
};

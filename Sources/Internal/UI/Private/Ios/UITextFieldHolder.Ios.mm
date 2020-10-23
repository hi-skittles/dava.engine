#include "UI/Private/Ios/UITextFieldHolder.Ios.h"

#if defined(__DAVAENGINE_IPHONE__)

#include "Engine/Engine.h"
#include "Engine/Window.h"
#include "Logger/Logger.h"
#include "Render/2D/Systems/VirtualCoordinatesSystem.h"
#include "UI/UIControlSystem.h"
#include "UI/Private/Ios/TextFieldPlatformImpl.Ios.h"
#include "Utils/NSStringUtils.h"
#include "Utils/StringUtils.h"
#include "Utils/UTF8Utils.h"

@implementation UITextFieldHolder

- (id)init
{
    if (self = [super init])
    {
        isKeyboardHidden = true;
        window = nullptr;

        self.userInteractionEnabled = TRUE;
        textInputAllowed = YES;
        useRtlAlign = NO;

        textCtrl = [[UITextField alloc] initWithFrame:CGRectMake(0.f, 0.f, 0.f, 0.f)];
        [textCtrl setValue:self forKey:@"delegate"];

        [self setupTraits];

        textCtrl.userInteractionEnabled = NO;

        cachedText = [[NSString alloc] initWithString:[textCtrl valueForKey:@"text"]];

        if ([textCtrl respondsToSelector:@selector(addTarget:action:forControlEvents:)])
        {
            [(id)textCtrl addTarget:self
                             action:@selector(eventEditingChanged:)
                   forControlEvents:UIControlEventEditingChanged];
        }

        textField = nullptr;

        // Done!
        [self addSubview:textCtrl];
    }
    return self;
}

- (void)attachWindow:(DAVA::Window*)w
{
    window = w;

    DAVA::Size2i inputScreenSize = DAVA::GetEngineContext()->uiControlSystem->vcs->GetInputScreenSize();

    self.bounds = CGRectMake(0.0f, 0.0f, inputScreenSize.dx, inputScreenSize.dy);
    self.center = CGPointMake(inputScreenSize.dx / 2.f, inputScreenSize.dy / 2.f);
}

- (void)setTextField:(DAVA::UITextField*)tf
{
    cppTextField = tf;
    if (tf)
    {
        DAVA::Rect physicalRect = DAVA::GetEngineContext()->uiControlSystem->vcs->ConvertVirtualToPhysical(tf->GetRect());
        DAVA::Vector2 physicalOffset = DAVA::GetEngineContext()->uiControlSystem->vcs->GetPhysicalDrawOffset();
        CGRect nativeRect = CGRectMake((physicalRect.x + physicalOffset.x),
                                       (physicalRect.y + physicalOffset.y),
                                       physicalRect.dx,
                                       physicalRect.dy);

        textCtrl.frame = nativeRect;
    }
    else
    {
        textCtrl.frame = CGRectMake(0.f, 0.f, 0.f, 0.f);
    }
}

- (void)textFieldDidBeginEditing:(UITextField*)textField
{
    if (cppTextField)
    {
        if (DAVA::GetEngineContext()->uiControlSystem->GetFocusedControl() != cppTextField)
        {
            DAVA::GetEngineContext()->uiControlSystem->SetFocusedControl(cppTextField);
        }
        if (!cppTextField->IsEditing())
        {
            cppTextField->StartEdit();
        }
    }
}

// next method(hitTest) fix sometime field editing(do not remove it)
- (id)hitTest:(CGPoint)point withEvent:(UIEvent*)event
{
    id hitView = [super hitTest:point withEvent:event];
    if (hitView == self)
        return nil;
    else
        return hitView;
}

- (void)textViewDidBeginEditing:(UITextView*)textView
{
    if (cppTextField)
    {
        if (DAVA::GetEngineContext()->uiControlSystem->GetFocusedControl() != cppTextField)
        {
            DAVA::GetEngineContext()->uiControlSystem->SetFocusedControl(cppTextField);
        }
        if (!cppTextField->IsEditing())
        {
            cppTextField->StartEdit();
        }
    }
}

- (void)dealloc
{
    [cachedText release];
    cachedText = nullptr;
    [textCtrl release];
    textCtrl = nullptr;
    [textField release];
    textField = nullptr;

    [[NSNotificationCenter defaultCenter] removeObserver:self];

    [super dealloc];
}

- (BOOL)textFieldShouldReturn:(UITextField*)textField
{
    if (cppTextField)
    {
        if (cppTextField->GetDelegate() != 0)
            cppTextField->GetDelegate()->TextFieldShouldReturn(cppTextField);
    }
    return TRUE;
}

- (BOOL)textField:(UITextField*)textField_ shouldChangeCharactersInRange:(NSRange)range replacementString:(NSString*)string
{
    DAVA::int32 maxLength = -1;
    if (nullptr != cppTextField)
    {
        maxLength = cppTextField->GetMaxLength();
    }

    BOOL applyChanges = YES;
    // Get string after changing
    NSString* origString = [textCtrl valueForKey:@"text"];
    NSString* replStr = string;

    if ([replStr length] > 0)
    {
        applyChanges = !DAVA::NSStringModified(range, origString, maxLength, &replStr);
        if (!applyChanges)
        {
            // remove all changes for undoMansger
            NSUndoManager* undoManager = [textField_ undoManager];
            [undoManager removeAllActions];
        }
    }
    // if we revert insert text
    if (range.location + range.length > [origString length])
    {
        range.length = [origString length] - range.location;
        applyChanges = NO;
    }

    BOOL clientApply = NO;
    if (nullptr != cppTextField && nullptr != cppTextField->GetDelegate())
    {
        if (range.length > 0 || [replStr length] > 0)
        {
            DAVA::WideString clientString = DAVA::WideStringFromNSString(replStr);
            clientApply = cppTextField->GetDelegate()->TextFieldKeyPressed(cppTextField, static_cast<DAVA::int32>(range.location), static_cast<DAVA::int32>(range.length), clientString);
        }
        if (!clientApply)
        {
            return NO;
        }
    }
    if (!applyChanges)
    {
        NSString* newString = [origString stringByReplacingCharactersInRange:range withString:replStr];
        [textCtrl setValue:newString forKey:@"text"];
        UITextPosition* caret = textField_.beginningOfDocument;
        caret = [textField_ positionFromPosition:caret offset:(range.location + [replStr length])];
        UITextRange* rangeCaret = [textField_ textRangeFromPosition:caret toPosition:caret];
        [textField_ setSelectedTextRange:rangeCaret];
        if (nullptr != cppTextField && nullptr != cppTextField->GetDelegate() && ![origString isEqualToString:newString])
        {
            DAVA::WideString clientString = DAVA::WideStringFromNSString(newString);
            DAVA::WideString oldString = DAVA::WideStringFromNSString(origString);
            cppTextField->GetDelegate()->TextFieldOnTextChanged(cppTextField, clientString, oldString, DAVA::UITextFieldDelegate::eReason::CODE);
        }
    }
    return applyChanges;
}

- (BOOL)textView:(UITextView*)textView_ shouldChangeTextInRange:(NSRange)range replacementText:(NSString*)string
{
    BOOL result = [self textField:(UITextField*)textView_ shouldChangeCharactersInRange:range replacementString:string];
    return result;
}

- (void)eventEditingChanged:(UIView*)sender
{
    NSString* fieldText = [textCtrl valueForKey:@"text"];

    if (sender == textCtrl
        && cppTextField
        && cppTextField->GetDelegate()
        && ![cachedText isEqualToString:fieldText])
    {
        DAVA::WideString oldString;
        const char* cstr = [cachedText cStringUsingEncoding:NSUTF8StringEncoding];
        DAVA::UTF8Utils::EncodeToWideString((DAVA::uint8*)cstr, (DAVA::int32)strlen(cstr), oldString);

        // Workaround: Additional check on the maximum length for cases
        // where system event shouldChangeCharactersInRange does not work for Asian keyboards
        //
        // It also helps in case of multistage text input (like chinese-simplified),
        // since iOS can append spaces between symbols for non-commited text
        // Truncate text only if user is not in multistage input (markedTextRange is nil)
        // It prevents commiting it,
        // and also avoids crash on iOS7 when setText is called during multistage input
        id<UITextInput> inputObject = (id<UITextInput>)textCtrl;
        int maxLength = cppTextField->GetMaxLength();
        if (maxLength > 0 && (int)fieldText.length > maxLength && ([inputObject markedTextRange] == nil))
        {
            fieldText = [fieldText substringToIndex:maxLength];

            if ([textCtrl class] == [ ::UITextField class])
            {
                auto textFieldPtr = (::UITextField*)textCtrl;
                auto selection = [textFieldPtr selectedTextRange];
                [textFieldPtr setText:fieldText];
                [textFieldPtr setSelectedTextRange:selection];
            }
            else
            {
                auto textViewPtr = (::UITextView*)textCtrl;
                auto selection = [textViewPtr selectedTextRange];
                [textViewPtr setText:fieldText];
                [textViewPtr setSelectedTextRange:selection];
            }
        }
        // End workaround

        [cachedText release];
        cachedText = [[NSString alloc] initWithString:fieldText];

        DAVA::WideString newString;
        cstr = [cachedText cStringUsingEncoding:NSUTF8StringEncoding];
        DAVA::UTF8Utils::EncodeToWideString((DAVA::uint8*)cstr, (DAVA::int32)strlen(cstr), newString);

        cppTextField->GetDelegate()->TextFieldOnTextChanged(cppTextField, newString, oldString, DAVA::UITextFieldDelegate::eReason::USER);
    }
}

- (void)textViewDidChange:(UITextView*)textView
{
    [self eventEditingChanged:textView];
}

- (BOOL)textFieldShouldBeginEditing:(UITextField*)textField
{
    return textInputAllowed;
}

- (BOOL)textViewShouldBeginEditing:(UITextView*)textView
{
    return textInputAllowed;
}

- (void)dropCachedText
{
    [cachedText release];
    cachedText = [[NSString alloc] initWithString:[textCtrl valueForKey:@"text"]];
}

- (void)setIsPassword:(bool)isPassword
{
    if ([textCtrl respondsToSelector:@selector(setSecureTextEntry:)])
    {
        [(id)textCtrl setSecureTextEntry:isPassword ? YES : NO];
    }
}

- (void)setTextInputAllowed:(bool)value
{
    textInputAllowed = (value == true);
}

- (void)setUseRtlAlign:(bool)value
{
    useRtlAlign = (value == true);

    if ([textCtrl class] == [ ::UITextField class])
    {
        ::UITextField* textFieldPtr = (::UITextField*)textCtrl;
        // Set natural alignment if need
        switch (textFieldPtr.contentHorizontalAlignment)
        {
        case UIControlContentHorizontalAlignmentLeft:
            textFieldPtr.textAlignment = useRtlAlign ? NSTextAlignmentNatural : NSTextAlignmentLeft;
            break;
        case UIControlContentHorizontalAlignmentRight:
            textFieldPtr.textAlignment = useRtlAlign ? NSTextAlignmentNatural : NSTextAlignmentRight;
            break;

        default:
            break;
        }
    }
    else
    {
        DAVA::Logger::Error("UITextField::setUseRtlAlign not work in multiline");
    }
}

- (void)setupTraits
{
    if (!cppTextField || !textCtrl)
    {
        return;
    }

    if ([textCtrl class] == [ ::UITextView class])
    {
        ::UITextView* textView = (::UITextView*)textCtrl;

        textView.autocapitalizationType = [self convertAutoCapitalizationType:(DAVA::UITextField::eAutoCapitalizationType)cppTextField->GetAutoCapitalizationType()];
        textView.autocorrectionType = [self convertAutoCorrectionType:(DAVA::UITextField::eAutoCorrectionType)cppTextField->GetAutoCorrectionType()];
        
#if __IPHONE_OS_VERSION_MIN_REQUIRED >= __IPHONE_5_0
        textView.spellCheckingType = [self convertSpellCheckingType:(DAVA::UITextField::eSpellCheckingType)cppTextField->GetSpellCheckingType()];
#endif
        textView.enablesReturnKeyAutomatically = [self convertEnablesReturnKeyAutomatically:cppTextField->IsEnableReturnKeyAutomatically()];
        textView.keyboardAppearance = [self convertKeyboardAppearanceType:(DAVA::UITextField::eKeyboardAppearanceType)cppTextField->GetKeyboardAppearanceType()];
        textView.keyboardType = [self convertKeyboardType:(DAVA::UITextField::eKeyboardType)cppTextField->GetKeyboardType()];
        textView.returnKeyType = [self convertReturnKeyType:(DAVA::UITextField::eReturnKeyType)cppTextField->GetReturnKeyType()];
    }
    else
    {
        ::UITextField* textFieldPtr = (::UITextField*)textCtrl;

        textFieldPtr.autocapitalizationType = [self convertAutoCapitalizationType:(DAVA::UITextField::eAutoCapitalizationType)cppTextField->GetAutoCapitalizationType()];
        textFieldPtr.autocorrectionType = [self convertAutoCorrectionType:(DAVA::UITextField::eAutoCorrectionType)cppTextField->GetAutoCorrectionType()];
        
#if __IPHONE_OS_VERSION_MIN_REQUIRED >= __IPHONE_5_0
        textFieldPtr.spellCheckingType = [self convertSpellCheckingType:(DAVA::UITextField::eSpellCheckingType)cppTextField->GetSpellCheckingType()];
#endif
        textFieldPtr.enablesReturnKeyAutomatically = [self convertEnablesReturnKeyAutomatically:cppTextField->IsEnableReturnKeyAutomatically()];
        textFieldPtr.keyboardAppearance = [self convertKeyboardAppearanceType:(DAVA::UITextField::eKeyboardAppearanceType)cppTextField->GetKeyboardAppearanceType()];
        textFieldPtr.keyboardType = [self convertKeyboardType:(DAVA::UITextField::eKeyboardType)cppTextField->GetKeyboardType()];
        textFieldPtr.returnKeyType = [self convertReturnKeyType:(DAVA::UITextField::eReturnKeyType)cppTextField->GetReturnKeyType()];
    }
}

- (UITextAutocapitalizationType)convertAutoCapitalizationType:(DAVA::UITextField::eAutoCapitalizationType)davaType
{
    switch (davaType)
    {
    case DAVA::UITextField::AUTO_CAPITALIZATION_TYPE_NONE:
    {
        return UITextAutocapitalizationTypeNone;
    }

    case DAVA::UITextField::AUTO_CAPITALIZATION_TYPE_WORDS:
    {
        return UITextAutocapitalizationTypeWords;
    }

    case DAVA::UITextField::AUTO_CAPITALIZATION_TYPE_ALL_CHARS:
    {
        return UITextAutocapitalizationTypeAllCharacters;
    }

    case DAVA::UITextField::AUTO_CAPITALIZATION_TYPE_SENTENCES:
    default:
    {
        // This is default one for iOS.
        return UITextAutocapitalizationTypeSentences;
    }
    }
}

- (UITextAutocorrectionType)convertAutoCorrectionType:(DAVA::UITextField::eAutoCorrectionType)davaType
{
    switch (davaType)
    {
    case DAVA::UITextField::AUTO_CORRECTION_TYPE_NO:
    {
        return UITextAutocorrectionTypeNo;
    }

    case DAVA::UITextField::AUTO_CORRECTION_TYPE_YES:
    {
        return UITextAutocorrectionTypeYes;
    }

    case DAVA::UITextField::AUTO_CORRECTION_TYPE_DEFAULT:
    default:
    {
        return UITextAutocorrectionTypeDefault;
    }
    }
}

- (UITextSpellCheckingType)convertSpellCheckingType:(DAVA::UITextField::eSpellCheckingType)davaType
{
    switch (davaType)
    {
    case DAVA::UITextField::SPELL_CHECKING_TYPE_NO:
    {
        return UITextSpellCheckingTypeNo;
    }

    case DAVA::UITextField::SPELL_CHECKING_TYPE_YES:
    {
        return UITextSpellCheckingTypeYes;
    }

    case DAVA::UITextField::SPELL_CHECKING_TYPE_DEFAULT:
    default:
    {
        return UITextSpellCheckingTypeDefault;
    }
    }
}

- (BOOL)convertEnablesReturnKeyAutomatically:(bool)davaType
{
    return (davaType ? YES : NO);
}

- (UIKeyboardAppearance)convertKeyboardAppearanceType:(DAVA::UITextField::eKeyboardAppearanceType)davaType
{
    switch (davaType)
    {
    case DAVA::UITextField::KEYBOARD_APPEARANCE_ALERT:
    {
        return UIKeyboardAppearanceAlert;
    }

    case DAVA::UITextField::KEYBOARD_APPEARANCE_DEFAULT:
    default:
    {
        return UIKeyboardAppearanceDefault;
    }
    }
}

- (UIKeyboardType)convertKeyboardType:(DAVA::UITextField::eKeyboardType)davaType
{
    switch (davaType)
    {
    case DAVA::UITextField::KEYBOARD_TYPE_ASCII_CAPABLE:
    {
        return UIKeyboardTypeASCIICapable;
    }

    case DAVA::UITextField::KEYBOARD_TYPE_NUMBERS_AND_PUNCTUATION:
    {
        return UIKeyboardTypeNumbersAndPunctuation;
    }

    case DAVA::UITextField::KEYBOARD_TYPE_URL:
    {
        return UIKeyboardTypeURL;
    }

    case DAVA::UITextField::KEYBOARD_TYPE_NUMBER_PAD:
    {
        return UIKeyboardTypeNumberPad;
    }

    case DAVA::UITextField::KEYBOARD_TYPE_PHONE_PAD:
    {
        return UIKeyboardTypePhonePad;
    }

    case DAVA::UITextField::KEYBOARD_TYPE_NAME_PHONE_PAD:
    {
        return UIKeyboardTypeNamePhonePad;
    }

    case DAVA::UITextField::KEYBOARD_TYPE_EMAIL_ADDRESS:
    {
        return UIKeyboardTypeEmailAddress;
    }

    case DAVA::UITextField::KEYBOARD_TYPE_DECIMAL_PAD:
    {
        return UIKeyboardTypeDecimalPad;
    }

    case DAVA::UITextField::KEYBOARD_TYPE_TWITTER:
    {
        return UIKeyboardTypeTwitter;
    }

    case DAVA::UITextField::KEYBOARD_TYPE_DEFAULT:
    default:
    {
        return UIKeyboardTypeDefault;
    }
    }
}

- (UIReturnKeyType)convertReturnKeyType:(DAVA::UITextField::eReturnKeyType)davaType
{
    switch (davaType)
    {
    case DAVA::UITextField::RETURN_KEY_GO:
    {
        return UIReturnKeyGo;
    }

    case DAVA::UITextField::RETURN_KEY_GOOGLE:
    {
        return UIReturnKeyGoogle;
    }

    case DAVA::UITextField::RETURN_KEY_JOIN:
    {
        return UIReturnKeyJoin;
    }

    case DAVA::UITextField::RETURN_KEY_NEXT:
    {
        return UIReturnKeyNext;
    }

    case DAVA::UITextField::RETURN_KEY_ROUTE:
    {
        return UIReturnKeyRoute;
    }

    case DAVA::UITextField::RETURN_KEY_SEARCH:
    {
        return UIReturnKeySearch;
    }

    case DAVA::UITextField::RETURN_KEY_SEND:
    {
        return UIReturnKeySend;
    }

    case DAVA::UITextField::RETURN_KEY_YAHOO:
    {
        return UIReturnKeyYahoo;
    }

    case DAVA::UITextField::RETURN_KEY_DONE:
    {
        return UIReturnKeyDone;
    }

    case DAVA::UITextField::RETURN_KEY_EMERGENCY_CALL:
    {
        return UIReturnKeyEmergencyCall;
    }

    case DAVA::UITextField::RETURN_KEY_DEFAULT:
    default:
    {
        return UIReturnKeyDefault;
    }
    }
}

- (void)keyboardFrameDidChange:(NSNotification*)notification
{
    // Remember the last keyboard frame here, since it might be incorrect in keyboardDidShow.
    lastKeyboardFrame = [[notification.userInfo objectForKey:UIKeyboardFrameEndUserInfoKey] CGRectValue];
}

- (void)keyboardWillHide:(NSNotification*)notification
{
    if (cppTextField && isKeyboardHidden == false)
    {
        cppTextField->OnKeyboardHidden();
        cppTextField->StopEdit();
        isKeyboardHidden = true;
    }
}

- (void)keyboardDidShow:(NSNotification*)notification
{
    if (nullptr == cppTextField || isKeyboardHidden == false)
    {
        return;
    }
    isKeyboardHidden = false;

    // convert own frame to window coordinates, frame is in superview's coordinates
    CGRect ownFrame = [textCtrl.window convertRect:self.frame fromView:textCtrl.superview];

    // calculate the area of own frame that is covered by keyboard
    CGRect keyboardFrame = CGRectIntersection(ownFrame, lastKeyboardFrame);

    // now this might be rotated, so convert it back
    keyboardFrame = [textCtrl.window convertRect:keyboardFrame toView:textCtrl.superview];

    // Recalculate to virtual coordinates.
    DAVA::Vector2 keyboardOrigin(keyboardFrame.origin.x, keyboardFrame.origin.y);
    keyboardOrigin = DAVA::GetEngineContext()->uiControlSystem->vcs->ConvertInputToVirtual(keyboardOrigin);

    DAVA::Vector2 keyboardSize(keyboardFrame.size.width, keyboardFrame.size.height);
    keyboardSize = DAVA::GetEngineContext()->uiControlSystem->vcs->ConvertInputToVirtual(keyboardSize);

    cppTextField->OnKeyboardShown(DAVA::Rect(keyboardOrigin, keyboardSize));
}

@end

#endif //#if defined(__DAVAENGINE_IPHONE__)

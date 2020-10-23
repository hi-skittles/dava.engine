package com.dava.engine;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Rect;
import android.os.IBinder;
import android.text.Editable;
import android.text.InputType;
import android.text.InputFilter;
import android.text.Spanned;
import android.text.Spannable;
import android.text.SpannableStringBuilder;
import android.text.TextUtils;
import android.text.TextWatcher;
import android.util.TypedValue;
import android.view.View;
import android.view.Gravity;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputMethodManager;
import android.widget.EditText;
import android.widget.TextView;

// Duplicates enum eAlign declared in Base/BaseTypes.h
class eAlign
{
    static final int ALIGN_LEFT = 0x01;
    static final int ALIGN_HCENTER = 0x02;
    static final int ALIGN_RIGHT = 0x04;
    static final int ALIGN_TOP = 0x08;
    static final int ALIGN_VCENTER = 0x10;
    static final int ALIGN_BOTTOM = 0x20;
};

// Duplicates UITextField's enums declared in UI/UITextField.h
class eAutoCapitalizationType
{
    static final int AUTO_CAPITALIZATION_TYPE_NONE = 0;
    static final int AUTO_CAPITALIZATION_TYPE_WORDS = 1;
    static final int AUTO_CAPITALIZATION_TYPE_SENTENCES = 2;
    static final int AUTO_CAPITALIZATION_TYPE_ALL_CHARS = 3;
}

class eAutoCorrectionType
{
    static final int AUTO_CORRECTION_TYPE_DEFAULT = 0;
    static final int AUTO_CORRECTION_TYPE_NO = 1;
    static final int AUTO_CORRECTION_TYPE_YES = 2;
}

class eSpellCheckingType
{
    static final int SPELL_CHECKING_TYPE_DEFAULT = 0;
    static final int SPELL_CHECKING_TYPE_NO = 1;
    static final int SPELL_CHECKING_TYPE_YES = 2;
}

class eKeyboardType
{
    static final int KEYBOARD_TYPE_DEFAULT = 0;
    static final int KEYBOARD_TYPE_ASCII_CAPABLE = 1;
    static final int KEYBOARD_TYPE_NUMBERS_AND_PUNCTUATION = 2;
    static final int KEYBOARD_TYPE_URL = 3;
    static final int KEYBOARD_TYPE_NUMBER_PAD = 4;
    static final int KEYBOARD_TYPE_PHONE_PAD = 5;
    static final int KEYBOARD_TYPE_NAME_PHONE_PAD = 6;
    static final int KEYBOARD_TYPE_EMAIL_ADDRESS = 7;
    static final int KEYBOARD_TYPE_DECIMAL_PAD = 8;
    static final int KEYBOARD_TYPE_TWITTER = 9;
}

class eReturnKeyType
{
    static final int RETURN_KEY_DEFAULT = 0;
    static final int RETURN_KEY_GO = 1;
    static final int RETURN_KEY_GOOGLE = 2;
    static final int RETURN_KEY_JOIN = 3;
    static final int RETURN_KEY_NEXT = 4;
    static final int RETURN_KEY_ROUTE = 5;
    static final int RETURN_KEY_SEARCH = 6;
    static final int RETURN_KEY_SEND = 7;
    static final int RETURN_KEY_YAHOO = 8;
    static final int RETURN_KEY_DONE = 9;
    static final int RETURN_KEY_EMERGENCY_CALL = 10;
}

final class DavaTextField implements TextWatcher,
                                     View.OnLongClickListener,
                                     View.OnFocusChangeListener,
                                     TextView.OnEditorActionListener,
                                     DavaKeyboardState.KeyboardStateListener
{
    private static final int CLOSE_KEYBOARD_DELAY = 30;

    // About java volatile https://docs.oracle.com/javase/tutorial/essential/concurrency/atomic.html
    private volatile long textfieldBackendPointer = 0;
    private DavaSurfaceView surfaceView = null;
    private CustomTextField nativeTextField = null;
    private InputFilter maxTextLengthFilter = null;

    // Properties that have been set in DAVA::Engine thread and waiting to apply to TextField
    private TextFieldProperties properties = new TextFieldProperties();
    boolean programmaticTextChange = false;

    // Some properties that reflect TextField current properties
    float x;
    float y;
    float width;
    float height;
    boolean multiline = false;
    boolean password = false;

    public static native void nativeReleaseWeakPtr(long backendPointer);
    public static native void nativeOnFocusChange(long backendPointer, boolean hasFocus);
    public static native void nativeOnKeyboardShown(long backendPointer, int x, int y, int w, int h);
    public static native void nativeOnKeyboardHidden(long backendPointer);
    public static native void nativeOnEnterPressed(long backendPointer);
    public static native boolean nativeOnKeyPressed(long backendPointer, int replacementStart, int replacementLength, String replaceWith);
    public static native void nativeOnTextChanged(long backendPointer, String newText, boolean programmaticTextChange);
    public static native void nativeOnTextureReady(long backendPointer, int[] pixels, int w, int h);

    private static boolean pendingKeyboardClose = false;

    // To use inside of DavaSurfaceView.onInputConnection
    private static int lastSelectedImeMode = 0;
    private static int lastSelectedInputType = 0;

    private class TextFieldProperties
    {
        TextFieldProperties() {}
        TextFieldProperties(TextFieldProperties other)
        {
            x = other.x;
            y = other.y;
            width = other.width;
            height = other.height;
            hasFocus = other.hasFocus;
            visible = other.visible;
            password = other.password;
            multiline = other.multiline;
            text = other.text;
            textColor = other.textColor;
            fontSize = other.fontSize;
            maxLength = other.maxLength;
            textAlign = other.textAlign;
            textRtlAlign = other.textRtlAlign;
            inputEnabled = other.inputEnabled;
            autoCapitalization = other.autoCapitalization;
            autoCorrection = other.autoCorrection;
            spellChecking = other.spellChecking;
            keyboardAppearance = other.keyboardAppearance;
            keyboardType = other.keyboardType;
            returnKeyType = other.returnKeyType;
            cursorPos = other.cursorPos;

            createNew = other.createNew;
            anyPropertyChanged = other.anyPropertyChanged;
            focusChanged = other.focusChanged;
            rectChanged = other.rectChanged;
            visibleChanged = other.visibleChanged;
            passwordChanged = other.passwordChanged;
            multilineChanged = other.multilineChanged;
            textChanged = other.textChanged;
            textColorChanged = other.textColorChanged;
            fontSizeChanged = other.fontSizeChanged;
            maxLengthChanged = other.maxLengthChanged;
            textAlignChanged = other.textAlignChanged;
            textRtlAlignChanged = other.textRtlAlignChanged;
            inputEnabledChanged = other.inputEnabledChanged;
            autoCapitalizationChanged = other.autoCapitalizationChanged;
            autoCorrectionChanged = other.autoCorrectionChanged;
            spellCheckingChanged = other.spellCheckingChanged;
            keyboardAppearanceChanged = other.keyboardAppearanceChanged;
            keyboardTypeChanged = other.keyboardTypeChanged;
            returnKeyTypeChanged = other.returnKeyTypeChanged;
            cursorPosChanged = other.cursorPosChanged;
        }

        float x;
        float y;
        float width;
        float height;
        boolean hasFocus;
        boolean visible;
        boolean password;
        boolean multiline;
        String text;
        float fontSize;
        int textColor;
        int maxLength;
        int textAlign = eAlign.ALIGN_LEFT | eAlign.ALIGN_TOP;
        boolean textRtlAlign = false;
        boolean inputEnabled;
        int autoCapitalization;
        int autoCorrection;
        int spellChecking;
        int keyboardAppearance;
        int keyboardType;
        int returnKeyType;
        int cursorPos;

        boolean createNew;
        boolean anyPropertyChanged;
        boolean focusChanged;
        boolean rectChanged;
        boolean visibleChanged;
        boolean passwordChanged;
        boolean multilineChanged;
        boolean textChanged;
        boolean fontSizeChanged;
        boolean textColorChanged;
        boolean maxLengthChanged;
        boolean textAlignChanged;
        boolean textRtlAlignChanged;
        boolean inputEnabledChanged;
        boolean autoCapitalizationChanged;
        boolean autoCorrectionChanged;
        boolean spellCheckingChanged;
        boolean keyboardAppearanceChanged;
        boolean keyboardTypeChanged;
        boolean returnKeyTypeChanged;
        boolean cursorPosChanged;

        void clearChangedFlags()
        {
            createNew = false;
            anyPropertyChanged = false;
            focusChanged = false;
            rectChanged = false;
            visibleChanged = false;
            passwordChanged = false;
            multilineChanged = false;
            textChanged = false;
            textColorChanged = false;
            fontSizeChanged = false;
            maxLengthChanged = false;
            textAlignChanged = false;
            textRtlAlignChanged = false;
            inputEnabledChanged = false;
            autoCapitalizationChanged = false;
            autoCorrectionChanged = false;
            spellCheckingChanged = false;
            keyboardAppearanceChanged = false;
            keyboardTypeChanged = false;
            returnKeyTypeChanged = false;
            cursorPosChanged = false;
        }
    }

    private class CustomTextField extends EditText
    {
        CustomTextField(Context context)
        {
            super(context);
        }

        @Override
        public boolean onKeyPreIme(int keyCode, KeyEvent event)
        {
            if (keyCode == KeyEvent.KEYCODE_BACK)
            {
                if (event.getAction() == KeyEvent.ACTION_DOWN)
                {
                    // skip ACTION_DOWN event
                }
                else
                {
                    clearFocus();
                }
                return true;
            }
            return super.onKeyPreIme(keyCode, event);
        }

        @Override
        public boolean onTouchEvent(MotionEvent event)
        {
            // Pass touch event to SurfaceView in single line mode
            if (!multiline)
            {
                MotionEvent newEvent = MotionEvent.obtain(event);
                newEvent.setLocation(getLeft() + event.getX(), getTop() + event.getY());
                surfaceView.dispatchTouchEvent(newEvent);
            }
            return super.onTouchEvent(event);
        }
    }

    public DavaTextField(DavaSurfaceView view, long backendPointer)
    {
        textfieldBackendPointer = backendPointer;
        surfaceView = view;

        properties.createNew = true;
        properties.anyPropertyChanged = true;
    }

    public static int getLastSelectedImeMode()
    {
        return lastSelectedImeMode;
    }

    public static int getLastSelectedInputType()
    {
        return lastSelectedInputType;
    }

    void release()
    {
        DavaActivity.commandHandler().post(new Runnable() {
            @Override public void run()
            {
                releaseNativeControl();
            }
        });
    }

    void setVisible(boolean visible)
    {
        if (properties.visible != visible)
        {
            properties.visible = visible;
            properties.visibleChanged = true;
            properties.anyPropertyChanged = true;
            if (!visible)
            {
                // Immediately hide native control
                DavaActivity.commandHandler().post(new Runnable() {
                    @Override public void run()
                    {
                        if (nativeTextField != null)
                        {
                            setNativeVisible(false);
                        }
                    }
                });
            }
        }
    }

    void setIsPassword(boolean password)
    {
        if (properties.password != password)
        {
            properties.password = password;
            properties.passwordChanged = true;
            properties.anyPropertyChanged = true;
        }
    }

    void setMaxLength(int value)
    {
        properties.maxLength = value;
        properties.maxLengthChanged = true;
        properties.anyPropertyChanged = true;
    }

    void openKeyboard()
    {
        properties.hasFocus = true;
        properties.focusChanged = true;
        properties.anyPropertyChanged = true;
    }

    void closeKeyboard()
    {
        properties.hasFocus = false;
        properties.focusChanged = true;
        properties.anyPropertyChanged = true;
    }

    void setRect(float x, float y, float width, float height)
    {
        properties.x = x;
        properties.y = y;
        properties.width = width;
        properties.height = height;
        properties.rectChanged = true;
        properties.anyPropertyChanged = true;
    }

    void setText(String text)
    {
        properties.text = text;
        properties.textChanged = true;
        properties.anyPropertyChanged = true;
    }

    void setTextColor(int a, int r, int g, int b)
    {
        properties.textColor = Color.argb(a, r, g, b);
        properties.textColorChanged = true;
        properties.anyPropertyChanged = true;
    }

    void setTextAlign(int align)
    {
        int gravityH = Gravity.LEFT;
        int gravityV = Gravity.TOP;
        if ((align & eAlign.ALIGN_LEFT) == eAlign.ALIGN_LEFT)
            gravityH = Gravity.LEFT;
        else if ((align & eAlign.ALIGN_HCENTER) == eAlign.ALIGN_HCENTER)
            gravityH = Gravity.CENTER_HORIZONTAL;
        else if ((align & eAlign.ALIGN_RIGHT) == eAlign.ALIGN_RIGHT)
            gravityH = Gravity.RIGHT;

        if ((align & eAlign.ALIGN_TOP) == eAlign.ALIGN_TOP)
            gravityV = Gravity.TOP;
        else if ((align & eAlign.ALIGN_VCENTER) == eAlign.ALIGN_VCENTER)
            gravityV = Gravity.CENTER_VERTICAL;
        else if ((align & eAlign.ALIGN_BOTTOM) == eAlign.ALIGN_BOTTOM)
            gravityV = Gravity.BOTTOM;

        properties.textAlign = gravityH | gravityV;
        properties.textAlignChanged = true;
        properties.anyPropertyChanged = true;
    }

    void setTextUseRtlAlign(boolean rtlAlign)
    {
        properties.textRtlAlign = rtlAlign;
        properties.textRtlAlignChanged = true;
        properties.anyPropertyChanged = true;
    }

    void setFontSize(float fontSize)
    {
        properties.fontSize = fontSize;
        properties.fontSizeChanged = true;
        properties.anyPropertyChanged = true;
    }

    void setMultiline(boolean multiline)
    {
        if (properties.multiline != multiline)
        {
            properties.multiline = multiline;
            properties.multilineChanged = true;
            properties.anyPropertyChanged = true;
        }
    }

    void setInputEnabled(boolean enable)
    {
        properties.inputEnabled = enable;
        properties.inputEnabledChanged = true;
        properties.anyPropertyChanged = true;
    }

    void setAutoCapitalizationType(int value)
    {
        properties.autoCapitalization = value;
        properties.autoCapitalizationChanged = true;
        properties.anyPropertyChanged = true;
    }

    void setAutoCorrectionType(int value)
    {
        properties.autoCorrection = value;
        properties.autoCorrectionChanged = true;
        properties.anyPropertyChanged = true;
    }

    void setSpellCheckingType(int value)
    {
        properties.spellChecking = value;
        properties.spellCheckingChanged = true;
        properties.anyPropertyChanged = true;
    }

    void setKeyboardAppearanceType(int value)
    {
        DavaLog.w(DavaActivity.LOG_TAG, "TextField: setKeyboardAppearanceType is not supported");
    }

    void setKeyboardType(int value)
    {
        properties.keyboardType = value;
        properties.keyboardTypeChanged = true;
        properties.anyPropertyChanged = true;
    }

    void setReturnKeyType(int value)
    {
        properties.returnKeyType = value;
        properties.returnKeyTypeChanged = true;
        properties.anyPropertyChanged = true;
    }

    void setEnableReturnKeyAutomatically(boolean value)
    {
        DavaLog.w(DavaActivity.LOG_TAG, "TextField: setEnableReturnKeyAutomatically is not supported");
    }

    int getCursorPos()
    {
        // TODO: is it safe to call this method from non-UI thread?
        // http://stackoverflow.com/questions/5962366/android-edittext-listener-for-cursor-position-change
        return nativeTextField.getSelectionStart();
    }

    void setCursorPos(int pos)
    {
        if (pos >= 0)
        {
            properties.cursorPos = pos;
            properties.cursorPosChanged = true;
            properties.anyPropertyChanged = true;
        }
    }

    void update()
    {
        if (properties.anyPropertyChanged)
        {
            final TextFieldProperties props = new TextFieldProperties(properties);
            DavaActivity.commandHandler().post(new Runnable() {
                @Override public void run()
                {
                    processProperties(props);
                }
            });
            properties.clearChangedFlags();
        }
    }

    void processProperties(TextFieldProperties props)
    {
        if (props.createNew)
        {
            createNativeControl();
        }

        if (nativeTextField != null)
        {
            if (props.anyPropertyChanged)
            {
                applyChangedProperties(props);
            }

            if (!multiline && !nativeTextField.hasFocus())
            {
                // We need make deferred rendering because updating layout of native field
                // will been applied on handler's next tick.
                DavaActivity.commandHandler().post(new Runnable() {
                    @Override public void run()
                    {
                        renderToTexture();
                    }
                });
            }
        }
    }

    void createNativeControl()
    {
        nativeTextField = new CustomTextField(DavaActivity.instance());
        nativeTextField.setSingleLine(true);
        nativeTextField.setBackgroundColor(Color.TRANSPARENT);
        nativeTextField.setTextColor(Color.WHITE);
        nativeTextField.setImeOptions(EditorInfo.IME_FLAG_NO_FULLSCREEN);
        nativeTextField.setDrawingCacheEnabled(true);
        nativeTextField.setPadding(0, 0, 0, 0);

        surfaceView.addControl(nativeTextField);

        DavaActivity.instance().keyboardState.addKeyboardStateListener(this);

        InputFilter inputFilter = new InputFilter() {
            @Override public CharSequence filter(CharSequence source,
                                                 int start,
                                                 int end,
                                                 Spanned dest,
                                                 int dstart,
                                                 int dend)
            {
                if (programmaticTextChange)
                {
                    return source;
                }

                // Workaround on lost focus filter called once more
                // so if in this moment we are loading/creating 
                // new TextField synchronously we will get deadlock
                // so if no changes to content just return
                if (start == 0 && end == 0 && dstart == dend)
                {
                    return null;
                }

                if (source instanceof Spanned)
                {
                    Spanned spanned = (Spanned)source;
                    SpannableStringBuilder spannableStringBuilder = new SpannableStringBuilder(source);
                    TextUtils.copySpansFrom(spanned, start, end, null, spannableStringBuilder, 0);
                    source = spannableStringBuilder;
                }

                if (maxTextLengthFilter != null)
                {
                    CharSequence resultSeq = maxTextLengthFilter.filter(source, start, end, dest, dstart, dend);
                    if (resultSeq != null && resultSeq.length() == 0)
                        return resultSeq;
                    if (resultSeq != null)
                        source = resultSeq;
                }
                boolean acceptText = nativeOnKeyPressed(textfieldBackendPointer, dstart, dend - dstart, source.toString());
                if (acceptText)
                {
                    return source;
                }
                return "";
        }};
        nativeTextField.setFilters(new InputFilter[] { inputFilter });

        nativeTextField.setOnLongClickListener(this);
        nativeTextField.setOnFocusChangeListener(this);
        nativeTextField.addTextChangedListener(this);
        nativeTextField.setOnEditorActionListener(this);
    }

    void releaseNativeControl()
    {
        nativeReleaseWeakPtr(textfieldBackendPointer);
        textfieldBackendPointer = 0;

        if (nativeTextField != null)
        {
            DavaActivity.instance().keyboardState.removeKeyboardStateListener(this);
            setNativeVisible(false);
            // Force onscreen keyboard hiding as on some phones keyboard stays on screen
            // allowing input which leads to null pointer access in nativeOnKeyPressed
            // called from InputFilter instance
            setNativeInputEnabled(false);
            surfaceView.removeControl(nativeTextField);
            nativeTextField = null;
        }
    }

    // View.OnLongClickListener interface
    @Override
    public boolean onLongClick(View v)
    {
        return !v.hasFocus();
    }

    // View.OnFocusChangeListener interface
    @Override
    public void onFocusChange(View v, boolean hasFocus)
    {
        if (hasFocus)
        {
            CustomTextField textField = (CustomTextField)v;
            lastSelectedImeMode = textField.getImeOptions();
            lastSelectedInputType = textField.getInputType();

            if (pendingKeyboardClose)
            {
                // If keyboard is still visible but not hidden yet - just cancel closing
                pendingKeyboardClose = false;
            }
            else
            {
                // Otherwise open it

                InputMethodManager imm = (InputMethodManager)DavaActivity.instance().getSystemService(Context.INPUT_METHOD_SERVICE);
                imm.showSoftInput(nativeTextField, InputMethodManager.SHOW_IMPLICIT);

                DavaKeyboardState keyboardState = DavaActivity.instance().keyboardState;
                if (keyboardState.isKeyboardOpen())
                {
                    Rect keyboardRect = keyboardState.keyboardRect();
                        nativeOnKeyboardShown(textfieldBackendPointer, keyboardRect.left, keyboardRect.top, keyboardRect.width(), keyboardRect.height());
                    }
                }
            }
        else
        {
            if (!pendingKeyboardClose)
            {
                // Close keyboard delayed (to avoid reopening it when switching between textfields)
                // If another textfield gets focus until that happens, it will cancel this action

                // Store windowToken in case textField will be detached from window
                final IBinder windowToken = nativeTextField.getWindowToken();

                DavaActivity.commandHandler().postDelayed(new Runnable()
                {
                    @Override
                    public void run()
                    {
                        if (!pendingKeyboardClose)
                        {
                            return;
                        }

                        InputMethodManager imm = (InputMethodManager) DavaActivity.instance().getSystemService(Context.INPUT_METHOD_SERVICE);
                        imm.hideSoftInputFromWindow(windowToken, 0);

                        if (textfieldBackendPointer != 0)
                        {
                            nativeOnKeyboardHidden(textfieldBackendPointer);
                        }

                        pendingKeyboardClose = false;
                    }
                }, CLOSE_KEYBOARD_DELAY);

                pendingKeyboardClose = true;
            }
        }

            nativeOnFocusChange(textfieldBackendPointer, hasFocus);

        if (!multiline)
        {
            setNativePositionAndSize(x, y, width, height, !hasFocus);
        }
    }

    // TextWatcher interface
    @Override
    public void onTextChanged(CharSequence s, int start, int before, int count)
    {
    }

    @Override
    public void beforeTextChanged(CharSequence s, int start, int count, int after)
    {
    }

    @Override
    public void afterTextChanged(Editable s)
    {
            nativeOnTextChanged(textfieldBackendPointer, s.toString(), programmaticTextChange);
        programmaticTextChange = false;
    }

    // TextView.OnEditorActionListener interface
    @Override
    public boolean onEditorAction(TextView v, int actionId, KeyEvent event)
    {
        if (!multiline)
        {
            nativeOnEnterPressed(textfieldBackendPointer);
            return true;
        }
        return false;
    }

    // DavaKeyboardState.KeyboardStateListener interface
    @Override
    public void onKeyboardOpened(Rect keyboardRect)
    {
        if (nativeTextField.hasFocus())
        {
            nativeOnKeyboardShown(textfieldBackendPointer, keyboardRect.left, keyboardRect.top, keyboardRect.width(), keyboardRect.height());
        }
    }

    @Override
    public void onKeyboardClosed()
    {
        if (nativeTextField.hasFocus())
        {
            nativeOnKeyboardHidden(textfieldBackendPointer);
        }
    }

    void applyChangedProperties(TextFieldProperties props)
    {
        if (props.autoCapitalizationChanged)
            setNativeAutoCapitalizationType(props.autoCapitalization);
        if (props.autoCorrectionChanged)
            setNativeAutoCorrectionType(props.autoCorrection);
        if (props.spellCheckingChanged)
            setNativeSpellCheckingType(props.spellChecking);
        if (props.keyboardTypeChanged)
            setNativeKeyboardType(props.keyboardType);
        if (props.returnKeyTypeChanged)
            setNativeReturnKeyType(props.returnKeyType);

        if (props.textAlignChanged || props.textRtlAlignChanged)
            setNativeTextAlign(props.textAlign, props.textRtlAlign);
        if (props.inputEnabledChanged)
            setNativeInputEnabled(props.inputEnabled);

        if (props.textChanged)
            setNativeText(props.text);
        if (props.fontSizeChanged)
            setNativeFontSize(props.fontSize);
        if (props.textColorChanged)
            setNativeTextColor(props.textColor);
        if (props.maxLengthChanged)
            setNativeMaxLength(props.maxLength);
        if (props.cursorPosChanged)
            setNativeCursorPos(props.cursorPos);

        if (props.passwordChanged)
            setNativePassword(props.password);
        if (props.multilineChanged)
        {
            multiline = props.multiline;
            setNativeMultiline(multiline);
        }
        if (props.rectChanged || props.visibleChanged)
        {
            x = props.x;
            y = props.y;
            width = props.width;
            height = props.height;
            setNativeVisible(props.visible);
            setNativePositionAndSize(x, y, width, height, !(multiline || nativeTextField.hasFocus()));
        }
        if (props.focusChanged)
            setNativeFocus(props.hasFocus);
    }

    void setNativePositionAndSize(float x, float y, float width, float height, boolean offScreen)
    {
        float xOffset = 0.0f;
        float yOffset = 0.0f;
        if (offScreen)
        {
            // Move control very far offscreen
            xOffset = x + width + 10000.0f;
            yOffset = y + height + 10000.0f;
        }
        surfaceView.positionControl(nativeTextField, x - xOffset, y - yOffset, width, height);
    }

    void setNativePassword(boolean password)
    {
        int inputType = nativeTextField.getInputType();
        if (password)
        {
            inputType = EditorInfo.TYPE_CLASS_TEXT | EditorInfo.TYPE_TEXT_VARIATION_PASSWORD;
        }
        else
        {
            inputType &= ~EditorInfo.TYPE_TEXT_VARIATION_PASSWORD;
        }

        int previousCursorPos = getCursorPos();
        programmaticTextChange = true; // setInputType might toggle filter to execute due to password transformation method
        nativeTextField.setInputType(inputType);
        programmaticTextChange = false; // In case filter didn't execute (i.e. when text is empty)

        // After transformation (if password is true) move cursor back
        setNativeCursorPos(previousCursorPos);
    }

    void setNativeMultiline(boolean multiline)
    {
        nativeTextField.setSingleLine(!multiline);
    }

    void setNativeVisible(boolean visible)
    {
        if (!visible)
            setNativeFocus(false);
        if (multiline)
        { // Multiline native text field is always onscreen according to visibiliy flag
            nativeTextField.setVisibility(visible ? View.VISIBLE : View.GONE);
        }
        else
        { // Single line native text field is always rendered to texture and placed offscreen when not visible
            setNativePositionAndSize(x, y, width, height, !visible);
        }
    }

    void setNativeFocus(boolean hasFocus)
    {
        if (hasFocus)
        {
            nativeTextField.requestFocus();
        }
        else
        {
            nativeTextField.clearFocus();
        }
    }

    void setNativeText(String text)
    {
        programmaticTextChange = true;
        nativeTextField.setText(text);
        // Move cursor to the end of text
        setNativeCursorPos(nativeTextField.getText().length());
    }

    void setNativeFontSize(float fontSize)
    {
        nativeTextField.setTextSize(TypedValue.COMPLEX_UNIT_PX, fontSize);
    }

    void setNativeTextColor(int color)
    {
        nativeTextField.setTextColor(color);
    }

    void setNativeMaxLength(int value)
    {
        if (value > 0)
        {
            maxTextLengthFilter = new InputFilter.LengthFilter(value);
        }
        else
        {
            maxTextLengthFilter = null;
        }
    }

    void setNativeTextAlign(int align, boolean rtlAlign)
    {
        boolean isCenter = (nativeTextField.getGravity() & Gravity.CENTER_HORIZONTAL) != 0;
        boolean isRelative = (nativeTextField.getGravity() & Gravity.RELATIVE_HORIZONTAL_GRAVITY_MASK) != 0;

        int gravity = align;
        if (!isCenter && (isRelative || rtlAlign))
        {
            gravity |= Gravity.RELATIVE_LAYOUT_DIRECTION;
        }
        nativeTextField.setGravity(gravity);
    }

    void setNativeInputEnabled(boolean enable)
    {
        nativeTextField.setEnabled(enable);
    }

    void setNativeCursorPos(int pos)
    {
        nativeTextField.setSelection(pos);
    }

    void setNativeAutoCapitalizationType(int value)
    {
        int flags = nativeTextField.getInputType();
        flags &= ~(InputType.TYPE_TEXT_FLAG_CAP_WORDS |
                   InputType.TYPE_TEXT_FLAG_CAP_SENTENCES |
                   InputType.TYPE_TEXT_FLAG_CAP_CHARACTERS);
        switch (value)
        {
        case eAutoCapitalizationType.AUTO_CAPITALIZATION_TYPE_WORDS:
            flags |= InputType.TYPE_TEXT_FLAG_CAP_WORDS;
            break;
        case eAutoCapitalizationType.AUTO_CAPITALIZATION_TYPE_SENTENCES:
            flags |= InputType.TYPE_TEXT_FLAG_CAP_SENTENCES;
            break;
        case eAutoCapitalizationType.AUTO_CAPITALIZATION_TYPE_ALL_CHARS:
            flags |= InputType.TYPE_TEXT_FLAG_CAP_CHARACTERS;
            break;
        case eAutoCapitalizationType.AUTO_CAPITALIZATION_TYPE_NONE:
        default:
            break;
        }
        nativeTextField.setInputType(flags);
    }

    void setNativeAutoCorrectionType(int value)
    {
        int flags = nativeTextField.getInputType();
        flags &= ~InputType.TYPE_TEXT_FLAG_AUTO_CORRECT;
        switch (value)
        {
        case eAutoCorrectionType.AUTO_CORRECTION_TYPE_DEFAULT:
        case eAutoCorrectionType.AUTO_CORRECTION_TYPE_YES:
            flags |= InputType.TYPE_TEXT_FLAG_AUTO_CORRECT;
            break;
        case eAutoCorrectionType.AUTO_CORRECTION_TYPE_NO:
        default:
            break;
        }
        nativeTextField.setInputType(flags);
    }

    void setNativeSpellCheckingType(int value)
    {
        int flags = nativeTextField.getInputType();
        flags &= ~InputType.TYPE_TEXT_FLAG_AUTO_COMPLETE;
        switch (value)
        {
        case eSpellCheckingType.SPELL_CHECKING_TYPE_DEFAULT:
        case eSpellCheckingType.SPELL_CHECKING_TYPE_YES:
            flags |= InputType.TYPE_TEXT_FLAG_AUTO_COMPLETE;
            break;
        case eSpellCheckingType.SPELL_CHECKING_TYPE_NO:
        default:
            break;
        }
        nativeTextField.setInputType(flags);
    }

    void setNativeKeyboardType(int value)
    {
        int flags = nativeTextField.getInputType();
        flags &= ~(InputType.TYPE_CLASS_NUMBER |
                   InputType.TYPE_CLASS_TEXT |
                   InputType.TYPE_TEXT_VARIATION_URI |
                   InputType.TYPE_TEXT_VARIATION_EMAIL_ADDRESS);
        switch (value)
        {
        case eKeyboardType.KEYBOARD_TYPE_NUMBERS_AND_PUNCTUATION:
        case eKeyboardType.KEYBOARD_TYPE_NUMBER_PAD:
        case eKeyboardType.KEYBOARD_TYPE_PHONE_PAD:
        case eKeyboardType.KEYBOARD_TYPE_NAME_PHONE_PAD:
        case eKeyboardType.KEYBOARD_TYPE_DECIMAL_PAD:
            flags |= InputType.TYPE_CLASS_NUMBER;
            break;
        case eKeyboardType.KEYBOARD_TYPE_URL:
        case eKeyboardType.KEYBOARD_TYPE_TWITTER:
            flags |= InputType.TYPE_CLASS_TEXT | InputType.TYPE_TEXT_VARIATION_URI;
            break;
        case eKeyboardType.KEYBOARD_TYPE_EMAIL_ADDRESS:
            flags |= InputType.TYPE_CLASS_TEXT | InputType.TYPE_TEXT_VARIATION_EMAIL_ADDRESS;
            break;
        case eKeyboardType.KEYBOARD_TYPE_DEFAULT:
        case eKeyboardType.KEYBOARD_TYPE_ASCII_CAPABLE:
        default:
            flags |= EditorInfo.TYPE_CLASS_TEXT;
            break;
        }
        nativeTextField.setInputType(flags);
    }

    void setNativeReturnKeyType(int value)
    {
        int options = EditorInfo.IME_FLAG_NO_FULLSCREEN;
        switch (value)
        {
        case eReturnKeyType.RETURN_KEY_GO:
            options |= EditorInfo.IME_ACTION_GO;
            break;
        case eReturnKeyType.RETURN_KEY_GOOGLE:
        case eReturnKeyType.RETURN_KEY_SEARCH:
        case eReturnKeyType.RETURN_KEY_YAHOO:
            options |= EditorInfo.IME_ACTION_SEARCH;
            break;
        case eReturnKeyType.RETURN_KEY_NEXT:
            options |= EditorInfo.IME_ACTION_NEXT;
            break;
        case eReturnKeyType.RETURN_KEY_SEND:
            options |= EditorInfo.IME_ACTION_SEND;
            break;
        case eReturnKeyType.RETURN_KEY_DEFAULT:
        case eReturnKeyType.RETURN_KEY_JOIN:
        case eReturnKeyType.RETURN_KEY_ROUTE:
        case eReturnKeyType.RETURN_KEY_DONE:
        case eReturnKeyType.RETURN_KEY_EMERGENCY_CALL:
        default:
            options |= EditorInfo.IME_ACTION_DONE;
            break;
        }
        nativeTextField.setImeOptions(options);
    }

    void renderToTexture()
    {
        if (nativeTextField == null)
        {
            return;
        }

        nativeTextField.setDrawingCacheEnabled(true);
        nativeTextField.buildDrawingCache();

        Bitmap bitmap = nativeTextField.getDrawingCache();
        if (bitmap == null) {
            int specWidth = View.MeasureSpec.makeMeasureSpec((int) width, View.MeasureSpec.EXACTLY);
            int specHeight = View.MeasureSpec.makeMeasureSpec((int) height, View.MeasureSpec.EXACTLY);
            nativeTextField.measure(specWidth, specHeight);
            int measuredWidth = nativeTextField.getMeasuredWidth();
            int measuredHeight = nativeTextField.getMeasuredHeight();

            bitmap = Bitmap.createBitmap(measuredWidth, measuredHeight, Bitmap.Config.ARGB_8888);
            Canvas canvas = new Canvas(bitmap);
            nativeTextField.layout(0, 0, measuredWidth, measuredHeight);
            nativeTextField.draw(canvas);
        }

        int w = bitmap.getWidth();
        int h = bitmap.getHeight();
        int[] pixels = new int[w * h];
        bitmap.getPixels(pixels, 0, w, 0, 0, w, h);

        nativeTextField.destroyDrawingCache();
        nativeTextField.setDrawingCacheEnabled(false);

        nativeOnTextureReady(textfieldBackendPointer, pixels, w, h);
    }
}

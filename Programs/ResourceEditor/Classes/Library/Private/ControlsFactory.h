#pragma once

#include "DAVAEngine.h"

class ControlsFactory
{
public:
    enum eGeneralControlSizes
    {
        BUTTON_HEIGHT = 20,
        BUTTON_WIDTH = 80,

        LEFT_PANEL_WIDTH = 200,
        RIGHT_PANEL_WIDTH = 200,
        OUTPUT_PANEL_HEIGHT = 70,
        PREVIEW_PANEL_HEIGHT = 200,

        OFFSET = 10,

        ERROR_MESSAGE_HEIGHT = 30,

        TEXTURE_PREVIEW_HEIGHT = 100,
        TEXTURE_PREVIEW_WIDTH = 200,

        TOOLS_HEIGHT = 40,
        TOOL_BUTTON_SIDE = 32,

        CELL_HEIGHT = 20,
    };

    enum eColorPickerSizes
    {
        COLOR_MAP_SIDE = 202,
        COLOR_SELECTOR_WIDTH = 20,
        COLOR_PREVIEW_SIDE = 80,
    };

public:
    static void ReleaseFonts();

    static void AddBorder(DAVA::UIControl* c);

    static DAVA::UIButton* CreateButton(const DAVA::Rect& rect, const DAVA::WideString& buttonText, bool designers = false);
    static void CustomizeButton(DAVA::UIButton* btn, const DAVA::WideString& buttonText, bool designers = false);

    static DAVA::Font* GetFont();
    static DAVA::float32 GetFontNormalSize();
    static DAVA::float32 GetFontBigSize();
    static DAVA::Color GetColorError();

    static DAVA::UIControl* CreateLine(const DAVA::Rect& rect, DAVA::Color color);

    static void CustomizeDialogFreeSpace(DAVA::UIControl* c);
    static void CustomizeDialog(DAVA::UIControl* c);

    static DAVA::Font* font;
};

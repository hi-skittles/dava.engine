#pragma once

enum eArea
{
    AREAS_BEGIN,
    ROTATE_AREA = AREAS_BEGIN,
    TOP_LEFT_AREA,
    TOP_CENTER_AREA,
    TOP_RIGHT_AREA,
    CENTER_LEFT_AREA,
    CENTER_RIGHT_AREA,
    BOTTOM_LEFT_AREA,
    BOTTOM_CENTER_AREA,
    BOTTOM_RIGHT_AREA,
    PIVOT_POINT_AREA,
    FRAME_AREA,
    NO_AREA,
    CORNERS_BEGIN = TOP_LEFT_AREA,
    CORNERS_COUNT = PIVOT_POINT_AREA - TOP_LEFT_AREA + CORNERS_BEGIN,
    AREAS_COUNT = NO_AREA - AREAS_BEGIN
};

//we have situations, when one input can produce two different state. To resolve this conflict we declare that state priority is equal to it value
//as an example dragging control with pressed space bar button will perform drag screen and transform at the same time
enum class eDragState
{
    NoDrag,
    Transform,
    SelectByRect,
    DragScreen,
    AddingControl,
    DuplicateByAlt
};

enum class eDisplayState
{
    //remove hud and throw all input to the DAVA framework
    Emulation,
    //just display all root controls, no other interaction enabled
    Preview,
    //display one root control
    Edit
};

//A client module can declare one or more own types and use them later, but can not use any other values
enum class eSystems
{
    //this system creates new controls above
    CREATING_CONTROLS,
    DUPLICATE_BY_ALT,
    //this system place root controls on the screen
    CONTROLS_VIEW,
    //this system move root control to it position. Controls positions are used by other systems, so this system must be updated before them
    CANVAS,
    //this system must be drawn in background of all other systems
    PIXEL_GRID,

    DISPLAY_FRAME,
    //this system creates HUD around controls and must be updated before HUD users
    HUD,
    //this system draw distance lines between controls
    DISTANCE_LINES,
    //this system can draw magnet lines and must be updated after the HUD system
    TRANSFORM,

    //Cursor system must be called after the HUD system to check current HUD area under cursor
    CURSOR,
    //this system doesn't require OnUpdate and don't create any controls. Can be less ordered than another systems
    SELECTION,
    //this system must be on bottom of all other systems, because modal control searching starting from end
    INPUT
};

enum class eInputSource
{
    SYSTEM,
    CANVAS
};

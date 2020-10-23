#pragma once

#include "Base/BaseTypes.h"

namespace ResourceEditor
{
enum eNodeType
{
    NODE_LANDSCAPE = 0,
    NODE_LIGHT,
    NODE_SERVICE_NODE,
    NODE_CAMERA,
    NODE_IMPOSTER,
    NODE_PARTICLE_EMITTER,
    NODE_USER_NODE,
    NODE_SWITCH_NODE,
    NODE_PARTICLE_EFFECT,

    NODE_COUNT
};

enum eViewportType
{
    VIEWPORT_IPHONE = 0,
    VIEWPORT_RETINA,
    VIEWPORT_IPAD,
    VIEWPORT_DEFAULT,

    VIEWPORT_COUNT
};

enum eHideableWidgets
{
    HIDABLEWIDGET_SCENE_GRAPH = 0,
    HIDABLEWIDGET_PROPERTIES,
    HIDABLEWIDGET_LIBRARY,
    HIDABLEWIDGET_TOOLBAR,
    HIDABLEWIDGET_REFERENCES,
    HIDABLEWIDGET_CUSTOMCOLORS,
    HIDEBLEWIDGET_VISIBILITYCHECKTOOL,
    HIDEBLEWIDGET_PARTICLE_EDITOR,
    HIDEBLEWIDGET_HANGINGOBJECTS,
    HIDEBLEWIDGET_SETSWITCHINDEX,
    HIDEBLEWIDGET_SCENEINFO,
    HIDABLEWIDGET_CUSTOMCOLORS2,
    HIDEBLEWIDGET_VISIBILITYCHECKTOOL2,
    HIDEBLEWIDGET_HEIGHTMAPEDITOR,
    HIDEBLEWIDGET_TILEMASKEDITOR,

    HIDABLEWIDGET_COUNT
};

enum eModificationActions
{
    MODIFY_NONE = 0,
    MODIFY_MOVE,
    MODIFY_ROTATE,
    MODIFY_SCALE,
    MODIFY_PLACE_ON_LAND,
    MODIFY_SNAP_TO_LAND,

    MODIFY_COUNT
};

enum eEditActions
{
    EDIT_UNDO,
    EDIT_REDO,

    EDIT_COUNT
};

// list: ["No Collision", "Tree", "Bush", "Fragile Proj", "Fragile ^Proj", "Falling", "Building", "Invisible Wall", "Speed Tree", "Undefined collision"]
enum eSceneObjectType
{
    ESOT_NONE = -1,
    ESOT_NO_COLISION,
    ESOT_TREE,
    ESOT_BUSH,
    ESOT_FRAGILE_PROJ,
    ESOT_FRAGILE_PROJ_INV,
    ESOT_FALLING,
    ESOT_BUILDING,
    ESOT_INVISIBLE_WALL,
    ESOT_SPEED_TREE,
    ESOT_UNDEFINED_COLLISION,

    ESOT_COUNT,
};

// coefficient for converting brush size from UI value to system value for landscape editors
const DAVA::float32 LANDSCAPE_BRUSH_SIZE_UI_TO_SYSTEM_COEF = 4.0f;

// default coefficient for converting brush size from UI value to system value for heightmap editors
// heightmap size in heightmap editor is almost 4 times smaller than landscape texture size
const DAVA::float32 HEIGHTMAP_BRUSH_SIZE_UI_TO_SYSTEM_COEF = 4.0f;

const DAVA::int32 SLIDER_WIDGET_CHANGE_VALUE_STEP_SMALL = 1;
const DAVA::int32 SLIDER_WIDGET_CHANGE_VALUE_STEP_LARGE = 10;

const DAVA::int32 BRUSH_MIN_BOUNDARY = 1;
const DAVA::int32 BRUSH_MAX_BOUNDARY = 999;

const DAVA::int32 DEFAULT_TOOLBAR_CONTROL_SIZE_WITH_TEXT = 150;
const DAVA::int32 DEFAULT_TOOLBAR_CONTROL_SIZE_WITH_ICON = 40;
};

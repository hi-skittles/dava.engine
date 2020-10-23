#pragma once

#include <Base/FastName.h>

#include <Base/FastName.h>

namespace ResourceEditor
{
// Node names
static const DAVA::FastName VEGETATION_NODE_NAME = DAVA::FastName("Vegetation");
static const DAVA::FastName LANDSCAPE_NODE_NAME = DAVA::FastName("Landscape");
static const DAVA::FastName LIGHT_NODE_NAME = DAVA::FastName("Light");
static const DAVA::FastName SERVICE_NODE_NAME = DAVA::FastName("Servicenode");
static const DAVA::FastName CAMERA_NODE_NAME = DAVA::FastName("Camera");
static const DAVA::FastName IMPOSTER_NODE_NAME = DAVA::FastName("Imposter");
static const DAVA::FastName PARTICLE_EMITTER_NODE_NAME = DAVA::FastName("Particle Emitter");
static const DAVA::FastName USER_NODE_NAME = DAVA::FastName("UserNode");
static const DAVA::FastName SWITCH_NODE_NAME = DAVA::FastName("SwitchNode");
static const DAVA::FastName PARTICLE_EFFECT_NODE_NAME = DAVA::FastName("Particle Effect");
static const DAVA::FastName WIND_NODE_NAME = DAVA::FastName("Wind");
static const DAVA::FastName PATH_NODE_NAME = DAVA::FastName("Path");
static const DAVA::String LAYER_NODE_NAME = "Layer";
static const DAVA::FastName ENTITY_NAME = DAVA::FastName("Entity");

// Base node names
static const DAVA::String EDITOR_BASE = "editor.";
static const DAVA::String EDITOR_MAIN_CAMERA = "editor.main-camera";
static const DAVA::FastName EDITOR_DEBUG_CAMERA = DAVA::FastName("editor.debug-camera");
static const DAVA::String EDITOR_ARROWS_NODE = "editor.arrows-node";
static const DAVA::FastName EDITOR_CAMERA_LIGHT = DAVA::FastName("editor.camera-light");
static const DAVA::FastName EDITOR_2D_CAMERA = DAVA::FastName("editor.2d-camera");
static const DAVA::FastName EDITOR_SPRITE = DAVA::FastName("Sprite");

// Headers
static const DAVA::WideString CREATE_NODE_LANDSCAPE = L"createnode.landscape";
static const DAVA::WideString CREATE_NODE_LIGHT = L"createnode.light";
static const DAVA::WideString CREATE_NODE_SERVICE = L"createnode.servicenode";
static const DAVA::WideString CREATE_NODE_CAMERA = L"createnode.camera";
static const DAVA::WideString CREATE_NODE_IMPOSTER = L"createnode.imposter";
static const DAVA::WideString CREATE_NODE_PARTICLE_EMITTER = L"createnode.particleemitter";
static const DAVA::WideString CREATE_NODE_USER = L"createnode.usernode";
static const DAVA::WideString CREATE_NODE_SWITCH = L"createnode.switchnode";
static const DAVA::WideString CREATE_NODE_PARTICLE_EFFECT = L"Particle Effect";

// Properties
static const DAVA::String EDITOR_REFERENCE_TO_OWNER = "editor.referenceToOwner";
static const DAVA::String EDITOR_CONST_REFERENCE = "editor.constReferent";
static const DAVA::String EDITOR_IS_LOCKED = "editor.isLocked";
static const DAVA::String EDITOR_DO_NOT_REMOVE = "editor.donotremove";
static const DAVA::String EDITOR_DYNAMIC_LIGHT_ENABLE = "editor.dynamiclight.enable";

//Documentation
static const DAVA::String DOCUMENTATION_PATH = "~doc:/ResourceEditorHelp/";

//service strings

static const DAVA::String CUSTOM_COLOR_TEXTURE_PROP = "customColorTexture";

static const DAVA::String TILEMASK_EDITOR_BRUSH_SIZE_CAPTION = "Brush\nsize:";
static const DAVA::String TILEMASK_EDITOR_BRUSH_IMAGE_CAPTION = "Brush\nimage:";
static const DAVA::String TILEMASK_EDITOR_DRAW_CAPTION = "Normal draw";
static const DAVA::String TILEMASK_EDITOR_COPY_PASTE_CAPTION = "Copy/paste";
static const DAVA::String TILEMASK_EDITOR_TILE_TEXTURE_CAPTION = "Tile texture:";
static const DAVA::String TILEMASK_EDITOR_STRENGTH_CAPTION = "Strength:";
static const DAVA::String TILEMASK_EDITOR_BRUSH_SIZE_MIN = "tilemask-editor.brush-size.min";
static const DAVA::String TILEMASK_EDITOR_BRUSH_SIZE_MAX = "tilemask-editor.brush-size.max";
static const DAVA::String TILEMASK_EDITOR_STRENGTH_MIN = "tilemask-editor.strength.min";
static const DAVA::String TILEMASK_EDITOR_STRENGTH_MAX = "tilemask-editor.strength.max";
static const DAVA::String TILEMASK_EDITOR_TOOLS_PATH = "~res:/ResourceEditor/LandscapeEditor/Tools/";
static const DAVA::String TILEMASK_EDITOR_ENABLE_ERROR = "Error enabling Tile Mask Editor. Make sure there is valid landscape at the scene.";
static const DAVA::String TILEMASK_EDITOR_DISABLE_ERROR = "Error disabling Tile Mask Editor";

static const DAVA::String CUSTOM_COLORS_BRUSH_SIZE_CAPTION = "Brush\nsize:";
static const DAVA::String CUSTOM_COLORS_BRUSH_SIZE_MIN = "custom-colors.brush-size.min";
static const DAVA::String CUSTOM_COLORS_BRUSH_SIZE_MAX = "custom-colors.brush-size.max";
static const DAVA::String CUSTOM_COLORS_PROPERTY_COLORS = "LandscapeCustomColors";
static const DAVA::String CUSTOM_COLORS_PROPERTY_DESCRIPTION = "LandscapeCustomColorsDescription";
static const DAVA::String CUSTOM_COLORS_SAVE_CAPTION = "Save texture";
static const DAVA::String CUSTOM_COLORS_LOAD_CAPTION = "Load texture";
static const DAVA::String CUSTOM_COLORS_ENABLE_ERROR = "Error enabling Custom Colors editor. Make sure there is valid landscape at the scene.";
static const DAVA::String CUSTOM_COLORS_DISABLE_ERROR = "Error disabling Custom Colors editor.";

static const DAVA::String VISIBILITY_TOOL_AREA_SIZE_CAPTION = "Visibility Area Size:";
static const DAVA::String VISIBILITY_TOOL_AREA_SIZE_MIN = "visibility-tool.area-size.min";
static const DAVA::String VISIBILITY_TOOL_AREA_SIZE_MAX = "visibility-tool.area-size.max";
static const DAVA::String VISIBILITY_TOOL_ENABLE_ERROR = "Error enabling Visibility Check Tool. Make sure there is valid landscape at the scene.";
static const DAVA::String VISIBILITY_TOOL_DISABLE_ERROR = "Error disabling Visibility Check Tool";
static const DAVA::String VISIBILITY_TOOL_SAVE_CAPTION = "Save visibility tool texture";
static const DAVA::String VISIBILITY_TOOL_ADD_POINT_CAPTION = "Add Visibility Point";
static const DAVA::String VISIBILITY_TOOL_SAVE_TEXTURE_CAPTION = "Save Texture";
static const DAVA::String VISIBILITY_TOOL_COMPUTE_VISIBILITY_CAPTION = "Compute Visibility";

static const DAVA::String RULER_TOOL_LENGTH_CAPTION = "Length:";
static const DAVA::String RULER_TOOL_PREVIEW_LENGTH_CAPTION = "Preview length:";
static const DAVA::String RULER_TOOL_ENABLE_ERROR = "Error enabling Ruler Tool. Make sure there is valid landscape at the scene.";
static const DAVA::String RULER_TOOL_DISABLE_ERROR = "Error disabling Ruler Tool";
static const DAVA::String LANDSCAPE_DIALOG_WRONG_PNG_ERROR = "PNG file should be in format A8 or A16.";

static const DAVA::String HEIGHTMAP_EDITOR_BRUSH_SIZE_CAPTION = "Brush\nsize:";
static const DAVA::String HEIGHTMAP_EDITOR_STRENGTH_CAPTION = "Strength:";
static const DAVA::String HEIGHTMAP_EDITOR_AVERAGE_STRENGTH_CAPTION = "Average\nstrength:";
static const DAVA::String HEIGHTMAP_EDITOR_BRUSH_SIZE_MIN = "heightmap-editor.brush-size.min";
static const DAVA::String HEIGHTMAP_EDITOR_BRUSH_SIZE_MAX = "heightmap-editor.brush-size.max";
static const DAVA::String HEIGHTMAP_EDITOR_STRENGTH_MAX = "heightmap-editor.strength.max";
static const DAVA::String HEIGHTMAP_EDITOR_AVERAGE_STRENGTH_MIN = "heightmap-editor.average-strength.min";
static const DAVA::String HEIGHTMAP_EDITOR_AVERAGE_STRENGTH_MAX = "heightmap-editor.average-strength.max";
static const DAVA::String HEIGHTMAP_EDITOR_TOOLS_PATH = "~res:/ResourceEditor/LandscapeEditor/Tools/";
static const DAVA::String HEIGHTMAP_EDITOR_ENABLE_ERROR = "Error enabling Height Map editor. Make sure there is valid landscape at the scene.";
static const DAVA::String HEIGHTMAP_EDITOR_DISABLE_ERROR = "Error disabling Height Map editor.";
static const DAVA::String HEIGHTMAP_EDITOR_RADIO_COPY_PASTE = "Copy/paste";
static const DAVA::String HEIGHTMAP_EDITOR_RADIO_ABS_DROP = "Abs & Drop";
static const DAVA::String HEIGHTMAP_EDITOR_RADIO_ABSOLUTE = "Absolute";
static const DAVA::String HEIGHTMAP_EDITOR_RADIO_AVERAGE = "Average";
static const DAVA::String HEIGHTMAP_EDITOR_RADIO_DROPPER = "Dropper";
static const DAVA::String HEIGHTMAP_EDITOR_RADIO_RELATIVE = "Relative";
static const DAVA::String HEIGHTMAP_EDITOR_CHECKBOX_HEIGHTMAP = "Height Map";
static const DAVA::String HEIGHTMAP_EDITOR_CHECKBOX_TILEMASK = "Tile Mask";
static const DAVA::String HEIGHTMAP_EDITOR_LABEL_BRUSH_IMAGE = "Brush\nimage:";
static const DAVA::String HEIGHTMAP_EDITOR_LABEL_DROPPER_HEIGHT = "Height:";

static const DAVA::String NOT_PASSABLE_TERRAIN_ENABLE_ERROR = "Error enabling Not Passable Terrain. Make sure there is valid landscape at the scene.";
static const DAVA::String NOT_PASSABLE_TERRAIN_DISABLE_ERROR = "Error disabling Not Passable Terrain";

static const DAVA::String SHORTCUT_BRUSH_SIZE_INCREASE_SMALL = "landscape-editor.brush-size.increase.small";
static const DAVA::String SHORTCUT_BRUSH_SIZE_DECREASE_SMALL = "landscape-editor.brush-size.decrease.small";
static const DAVA::String SHORTCUT_BRUSH_SIZE_INCREASE_LARGE = "landscape-editor.brush-size.increase.large";
static const DAVA::String SHORTCUT_BRUSH_SIZE_DECREASE_LARGE = "landscape-editor.brush-size.decrease.large";
static const DAVA::String SHORTCUT_BRUSH_IMAGE_NEXT = "landscape-editor.brush-image.next";
static const DAVA::String SHORTCUT_BRUSH_IMAGE_PREV = "landscape-editor.brush-image.prev";
static const DAVA::String SHORTCUT_TEXTURE_NEXT = "landscape-editor.texture.next";
static const DAVA::String SHORTCUT_TEXTURE_PREV = "landscape-editor.texture.prev";
static const DAVA::String SHORTCUT_STRENGTH_INCREASE_SMALL = "landscape-editor.strength.increase.small";
static const DAVA::String SHORTCUT_STRENGTH_DECREASE_SMALL = "landscape-editor.strength.decrease.small";
static const DAVA::String SHORTCUT_STRENGTH_INCREASE_LARGE = "landscape-editor.strength.increase.large";
static const DAVA::String SHORTCUT_STRENGTH_DECREASE_LARGE = "landscape-editor.strength.decrease.large";
static const DAVA::String SHORTCUT_AVG_STRENGTH_INCREASE_SMALL = "landscape-editor.average-strength.increase.small";
static const DAVA::String SHORTCUT_AVG_STRENGTH_DECREASE_SMALL = "landscape-editor.average-strength.decrease.small";
static const DAVA::String SHORTCUT_AVG_STRENGTH_INCREASE_LARGE = "landscape-editor.average-strength.increase.large";
static const DAVA::String SHORTCUT_AVG_STRENGTH_DECREASE_LARGE = "landscape-editor.average-strength.decrease.large";
static const DAVA::String SHORTCUT_SET_COPY_PASTE = "heightmap-editor.set-copy-paste";
static const DAVA::String SHORTCUT_SET_ABSOLUTE = "heightmap-editor.set-absolute";
static const DAVA::String SHORTCUT_SET_RELATIVE = "heightmap-editor.set-relative";
static const DAVA::String SHORTCUT_SET_AVERAGE = "heightmap-editor.set-average";
static const DAVA::String SHORTCUT_SET_ABS_DROP = "heightmap-editor.set-abs-drop";
static const DAVA::String SHORTCUT_SET_DROPPER = "heightmap-editor.set-dropper";
static const DAVA::String SHORTCUT_NORMAL_DRAW_TILEMASK = "tilemaskeditor-editor.set-normal-draw";
static const DAVA::String SHORTCUT_COPY_PASTE_TILEMASK = "tilemaskeditor-editor.set-copy-paste";

static const DAVA::String NO_LANDSCAPE_ERROR_MESSAGE = "Error. Check is there landscape at the scene.";
static const DAVA::String INVALID_LANDSCAPE_MESSAGE = "Error. Check if all necessary properties of the landscape are set.";

static const DAVA::String SCENE_NODE_DESIGNER_NAME_PROPERTY_NAME = "editor.designerName";
static const DAVA::String SCENE_NODE_MODIFICATION_DATA_PROPERTY_NAME = "editor.modificationData";

static const DAVA::String SLIDER_WIDGET_CHANGE_VALUE_TOOLTIP = "Double click to change value";
static const DAVA::String SLIDER_WIDGET_CURRENT_VALUE = "Current value";

static const DAVA::String TILE_TEXTURE_PREVIEW_CHANGE_COLOR_TOOLTIP = "Click to change color";

static const DAVA::String LANDSCAPE_EDITOR_SYSTEM_NO_ERRORS = "No errors.";
static const DAVA::String LANDSCAPE_EDITOR_SYSTEM_DISABLE_EDITORS = "Error: All another landscape editors should be disabled";
static const DAVA::String LANDSCAPE_EDITOR_SYSTEM_LANDSCAPE_ENTITY_ABSENT = "Error: landscape entity is absent.";
static const DAVA::String LANDSCAPE_EDITOR_SYSTEM_TILEMASK_TEXTURE_ABSETN = "Error: tile mask texture is absent.";
static const DAVA::String LANDSCAPE_EDITOR_SYSTEM_FULLTILED_TEXTURE_ABSETN = "Error: full tiled texture is absent.";
static const DAVA::String LANDSCAPE_EDITOR_SYSTEM_TILE_TEXTURE_ABSENT = "Error: tile texture is absent.";
static const DAVA::String LANDSCAPE_EDITOR_SYSTEM_COLOR_TEXTURE_ABSENT = "Error: color texture is absent.";
static const DAVA::String LANDSCAPE_EDITOR_SYSTEM_HEIGHTMAP_ABSENT = "Error: heightmap is absent.";
static const DAVA::String LANDSCAPE_EDITOR_SYSTEM_CUSTOMCOLORS_ABSENT = "Warning: custom color texture is absent. Default texture will be created.";

static const DAVA::String ADD_SWITCH_NODE_DIALOG_NO_CHILDREN = "Error: switch node must be created with state nodes.";
static const DAVA::String ADD_SWITCH_NODE_DIALOG_NO_RENDER_OBJECTS = "Error: entities should contain render components with mesh render objects.";
static const DAVA::String ADD_SWITCH_NODE_DIALOG_DENY_SRC_SWITCH = "Error: entities should not contain switch components";
static const DAVA::String SCENE_TREE_WRONG_REF_TO_OWNER = "Wrong reference(s) to owner: ";
}

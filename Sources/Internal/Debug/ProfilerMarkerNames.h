#pragma once

namespace DAVA
{
namespace ProfilerCPUMarkerName
{
//Engine flow
extern const char* ENGINE_ON_FRAME;
extern const char* ENGINE_DO_EVENTS;
extern const char* ENGINE_BEGIN_FRAME;
extern const char* ENGINE_END_FRAME;
extern const char* ENGINE_UPDATE;
extern const char* ENGINE_UPDATE_WINDOW;
extern const char* ENGINE_DRAW_WINDOW;

extern const char* JOB_MANAGER;
extern const char* SOUND_SYSTEM;
extern const char* ANIMATION_MANAGER;
extern const char* UI_UPDATE;
extern const char* UI_DRAW;

extern const char* UI_UPDATE_SYSTEM;
extern const char* UI_LAYOUT_SYSTEM;
extern const char* UI_STYLE_SHEET_SYSTEM;
extern const char* UI_TEXT_SYSTEM;
extern const char* UI_RENDER_SYSTEM;

extern const char* UI_TEXTBLOCK_RECALC_PARAMS;
extern const char* UI_TEXTBLOCK_PREPARE;

//Scene
extern const char* SCENE_UPDATE;
extern const char* SCENE_DRAW;
extern const char* SCENE_STATIC_OCCLUSION_SYSTEM;
extern const char* SCENE_ANIMATION_SYSTEM;
extern const char* SCENE_UPDATE_SYSTEM_PRE_TRANSFORM;
extern const char* SCENE_UPDATE_SYSTEM_POST_TRANSFORM;
extern const char* SCENE_TRANSFORM_SYSTEM;
extern const char* SCENE_LOD_SYSTEM;
extern const char* SCENE_SWITCH_SYSTEM;
extern const char* SCENE_PARTICLE_SYSTEM;
extern const char* SCENE_SOUND_UPDATE_SYSTEM;
extern const char* SCENE_RENDER_UPDATE_SYSTEM;
extern const char* SCENE_ACTION_UPDATE_SYSTEM;
extern const char* SCENE_DEBUG_RENDER_SYSTEM;
extern const char* SCENE_LANDSCAPE_SYSTEM;
extern const char* SCENE_FOLIAGE_SYSTEM;
extern const char* SCENE_SPEEDTREE_SYSTEM;
extern const char* SCENE_WIND_SYSTEM;
extern const char* SCENE_WAVE_SYSTEM;
extern const char* SCENE_SKELETON_SYSTEM;
extern const char* SCENE_MOTION_SYSTEM;
extern const char* SCENE_GEODECAL_SYSTEM;

//Render
extern const char* RENDER_PASS_PREPARE_ARRAYS;
extern const char* RENDER_PASS_DRAW_LAYERS;
extern const char* RENDER_PREPARE_LANDSCAPE;

//RHI
extern const char* RHI_RENDER_LOOP;
extern const char* RHI_EXECUTE_FRAME;
extern const char* RHI_PRESENT;
extern const char* RHI_DEVICE_PRESENT;
extern const char* RHI_EXECUTE_IMMEDIATE_CMDS;
extern const char* RHI_WAIT_IMMEDIATE_CMDS;
extern const char* RHI_WAIT_FRAME_EXECUTION;
extern const char* RHI_CMD_BUFFER_EXECUTE;
extern const char* RHI_WAIT_FRAME_CONSTRUCTION;
extern const char* RHI_PROCESS_SCHEDULED_DELETE;
};

namespace ProfilerGPUMarkerName
{
extern const char* GPU_FRAME;
extern const char* RENDER_PASS_2D;
extern const char* RENDER_PASS_MAIN_3D;
extern const char* RENDER_PASS_WATER_REFLECTION;
extern const char* RENDER_PASS_WATER_REFRACTION;
extern const char* LANDSCAPE;
};
}; //ns DAVA

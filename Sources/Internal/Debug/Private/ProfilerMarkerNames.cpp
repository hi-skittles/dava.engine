#include "Debug/ProfilerMarkerNames.h"

namespace DAVA
{
namespace ProfilerCPUMarkerName
{
//Engine flow
const char* ENGINE_ON_FRAME = "Engine::OnFrame";
const char* ENGINE_DO_EVENTS = "Engine::DoEvents";
const char* ENGINE_BEGIN_FRAME = "Engine::BeginFrame";
const char* ENGINE_END_FRAME = "Engine::EndFrame";
const char* ENGINE_UPDATE = "Engine::Update";
const char* ENGINE_UPDATE_WINDOW = "Engine::UpdateWindow";
const char* ENGINE_DRAW_WINDOW = "Engine::DrawWindow";

const char* JOB_MANAGER = "JobManager";
const char* SOUND_SYSTEM = "SoundSystem";
const char* ANIMATION_MANAGER = "AnimationManager";
const char* UI_UPDATE = "UI::Update";
const char* UI_DRAW = "UI::Draw";

const char* UI_UPDATE_SYSTEM = "UIUpdateSystem";
const char* UI_LAYOUT_SYSTEM = "UILayoutSystem";
const char* UI_STYLE_SHEET_SYSTEM = "UIStyleSheetSystem";
const char* UI_TEXT_SYSTEM = "UITextSystem";
const char* UI_RENDER_SYSTEM = "UIRenderSystem";

const char* UI_TEXTBLOCK_RECALC_PARAMS = "UI::TextBlock::CalculateParams";
const char* UI_TEXTBLOCK_PREPARE = "UI::TextBlock::Prepare";

//Scene
const char* SCENE_UPDATE = "Scene::Update";
const char* SCENE_DRAW = "Scene::Draw";
const char* SCENE_STATIC_OCCLUSION_SYSTEM = "StaticOcclusionSystem";
const char* SCENE_ANIMATION_SYSTEM = "AnimationSystem";
const char* SCENE_UPDATE_SYSTEM_PRE_TRANSFORM = "UpdateSystem::PreTransform";
const char* SCENE_UPDATE_SYSTEM_POST_TRANSFORM = "UpdateSystem::PostTransform";
const char* SCENE_TRANSFORM_SYSTEM = "TransformSystem";
const char* SCENE_LOD_SYSTEM = "LodSystem";
const char* SCENE_SWITCH_SYSTEM = "SwitchSystem";
const char* SCENE_PARTICLE_SYSTEM = "ParticleEffectSystem";
const char* SCENE_SOUND_UPDATE_SYSTEM = "SoundUpdateSystem";
const char* SCENE_RENDER_UPDATE_SYSTEM = "RenderUpdateSystem";
const char* SCENE_ACTION_UPDATE_SYSTEM = "ActionUpdateSystem";
const char* SCENE_DEBUG_RENDER_SYSTEM = "DebugRenderSystem";
const char* SCENE_LANDSCAPE_SYSTEM = "LandscapeSystem";
const char* SCENE_FOLIAGE_SYSTEM = "FoliageSystem";
const char* SCENE_SPEEDTREE_SYSTEM = "SpeedTreeUpdateSystem";
const char* SCENE_WIND_SYSTEM = "WindSystem";
const char* SCENE_WAVE_SYSTEM = "WaveSystem";
const char* SCENE_SKELETON_SYSTEM = "SkeletonSystem";
const char* SCENE_MOTION_SYSTEM = "MotionSystem";
const char* SCENE_GEODECAL_SYSTEM = "GeoDecalSystem";

//Render
const char* RENDER_PASS_PREPARE_ARRAYS = "RenderPass::PrepareArrays";
const char* RENDER_PASS_DRAW_LAYERS = "RenderPass::DrawLayers";
const char* RENDER_PREPARE_LANDSCAPE = "Landscape::Prepare";

//RHI
const char* RHI_RENDER_LOOP = "rhi::RenderLoop";
const char* RHI_EXECUTE_FRAME = "rhi::ExecuteFrame";
const char* RHI_PRESENT = "rhi::Present";
const char* RHI_DEVICE_PRESENT = "rhi::DevicePresent";
const char* RHI_EXECUTE_QUEUED_CMDS = "rhi::ExecuteQueuedCmds";
const char* RHI_EXECUTE_IMMEDIATE_CMDS = "rhi::ExecuteImmidiateCmds";
const char* RHI_WAIT_IMMEDIATE_CMDS = "rhi::WaitImmediateCmd";
const char* RHI_WAIT_FRAME_EXECUTION = "rhi::WaitFrameExecution";
const char* RHI_CMD_BUFFER_EXECUTE = "rhi::cb::Execute";
const char* RHI_WAIT_FRAME_CONSTRUCTION = "rhi::WaitFrameConstruction";
const char* RHI_PROCESS_SCHEDULED_DELETE = "rhi::ProcessScheduledDelete";
};

namespace ProfilerGPUMarkerName
{
const char* GPU_FRAME = "GPUFrame";
const char* RENDER_PASS_2D = "RenderPass2D";
const char* RENDER_PASS_MAIN_3D = "RenderPassMain3D";
const char* RENDER_PASS_WATER_REFLECTION = "RenderPassWaterRefl";
const char* RENDER_PASS_WATER_REFRACTION = "RenderPassWaterRefr";
const char* LANDSCAPE = "Landscape";
};
}; //ns DAVA

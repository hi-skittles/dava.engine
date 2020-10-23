#include "DAVAEngine.h"
#include "DAVAClassRegistrator.h"
#include "Render/Highlevel/ShadowVolume.h"
#include "Engine/Engine.h"

#if defined(__DAVAENGINE_PHYSICS_ENABLED__)
#include <Physics/StaticBodyComponent.h>
#include <Physics/DynamicBodyComponent.h>
#include <Physics/BoxShapeComponent.h>
#include <Physics/CapsuleShapeComponent.h>
#include <Physics/SphereShapeComponent.h>
#include <Physics/PlaneShapeComponent.h>
#include "Physics/ConvexHullShapeComponent.h"
#include <Physics/MeshShapeComponent.h>
#include <Physics/HeightFieldShapeComponent.h>
#include <Physics/VehicleCarComponent.h>
#include <Physics/VehicleTankComponent.h>
#include <Physics/VehicleChassisComponent.h>
#include <Physics/VehicleWheelComponent.h>
#include <Physics/BoxCharacterControllerComponent.h>
#include <Physics/CapsuleCharacterControllerComponent.h>
#include <Physics/WASDPhysicsControllerComponent.h>
#endif

void DAVA::RegisterDAVAClasses()
{
    // this code does nothing. Needed to compiler generate code from this cpp file
    Logger* log = GetEngineContext()->logger;
    if (log)
        log->Log(Logger::LEVEL__DISABLE, "");
}

REGISTER_CLASS_WITH_NAMESPACE(BaseObject, DAVA::);
REGISTER_CLASS_WITH_NAMESPACE(PolygonGroup, DAVA::);
REGISTER_CLASS_WITH_NAMESPACE(StaticMesh, DAVA::);
REGISTER_CLASS_WITH_NAMESPACE(Camera, DAVA::);
REGISTER_CLASS_WITH_NAMESPACE(UIScrollViewContainer, DAVA::);
REGISTER_CLASS_WITH_NAMESPACE(UISlider, DAVA::);
REGISTER_CLASS_WITH_NAMESPACE(UISpinner, DAVA::);
REGISTER_CLASS_WITH_NAMESPACE(UIStaticText, DAVA::);
REGISTER_CLASS_WITH_NAMESPACE(UISwitch, DAVA::);
REGISTER_CLASS_WITH_NAMESPACE(UITextField, DAVA::);
REGISTER_CLASS_WITH_NAMESPACE(Landscape, DAVA::);
REGISTER_CLASS_WITH_NAMESPACE(AnimationData, DAVA::);
REGISTER_CLASS_WITH_NAMESPACE(Light, DAVA::);
REGISTER_CLASS_WITH_NAMESPACE(Mesh, DAVA::);
REGISTER_CLASS_WITH_NAMESPACE(SkinnedMesh, DAVA::);
REGISTER_CLASS_WITH_NAMESPACE(SpeedTreeObject, DAVA::);
REGISTER_CLASS_WITH_NAMESPACE(RenderBatch, DAVA::);
REGISTER_CLASS_WITH_NAMESPACE(RenderObject, DAVA::);
REGISTER_CLASS_WITH_NAMESPACE(ShadowVolume, DAVA::);
REGISTER_CLASS_WITH_NAMESPACE(NMaterial, DAVA::);
REGISTER_CLASS_WITH_NAMESPACE(DataNode, DAVA::);
REGISTER_CLASS_WITH_NAMESPACE(Entity, DAVA::);
REGISTER_CLASS_WITH_NAMESPACE(Scene, DAVA::);
REGISTER_CLASS_WITH_NAMESPACE(UIButton, DAVA::);
REGISTER_CLASS_WITH_NAMESPACE(UIControl, DAVA::);
REGISTER_CLASS_WITH_NAMESPACE(UIList, DAVA::);
REGISTER_CLASS_WITH_NAMESPACE(UIListCell, DAVA::);
REGISTER_CLASS_WITH_NAMESPACE(UIScrollBar, DAVA::);
REGISTER_CLASS_WITH_NAMESPACE(UIScrollView, DAVA::);
REGISTER_CLASS_WITH_ALIAS_AND_NAMESPACE(PartilceEmitterLoadProxy, "ParticleEmitter3D", DAVA::);
REGISTER_CLASS_WITH_NAMESPACE(UIWebView, DAVA::);
REGISTER_CLASS_WITH_NAMESPACE(UIMovieView, DAVA::);
REGISTER_CLASS_WITH_NAMESPACE(UIParticles, DAVA::);
REGISTER_CLASS_WITH_NAMESPACE(UIJoypad, DAVA::);
REGISTER_CLASS_WITH_NAMESPACE(VegetationRenderObject, DAVA::);
REGISTER_CLASS_WITH_NAMESPACE(BillboardRenderObject, DAVA::);
REGISTER_CLASS_WITH_NAMESPACE(SpriteObject, DAVA::);
REGISTER_CLASS_WITH_NAMESPACE(UI3DView, DAVA::);
REGISTER_CLASS_WITH_NAMESPACE(AnimationComponent, DAVA::);
REGISTER_CLASS_WITH_NAMESPACE(TransformComponent, DAVA::);
REGISTER_CLASS_WITH_NAMESPACE(UpdatableComponent, DAVA::);
REGISTER_CLASS_WITH_NAMESPACE(RenderComponent, DAVA::);
REGISTER_CLASS_WITH_NAMESPACE(CustomPropertiesComponent, DAVA::);
REGISTER_CLASS_WITH_NAMESPACE(ActionComponent, DAVA::);
REGISTER_CLASS_WITH_NAMESPACE(DebugRenderComponent, DAVA::);
REGISTER_CLASS_WITH_NAMESPACE(SoundComponent, DAVA::);
REGISTER_CLASS_WITH_NAMESPACE(BulletComponent, DAVA::);
REGISTER_CLASS_WITH_NAMESPACE(LightComponent, DAVA::);
REGISTER_CLASS_WITH_NAMESPACE(SpeedTreeComponent, DAVA::);
REGISTER_CLASS_WITH_NAMESPACE(WindComponent, DAVA::);
REGISTER_CLASS_WITH_NAMESPACE(WaveComponent, DAVA::);
REGISTER_CLASS_WITH_NAMESPACE(QualitySettingsComponent, DAVA::);
REGISTER_CLASS_WITH_NAMESPACE(UserComponent, DAVA::);
REGISTER_CLASS_WITH_NAMESPACE(SwitchComponent, DAVA::);
REGISTER_CLASS_WITH_NAMESPACE(ParticleEffectComponent, DAVA::);
REGISTER_CLASS_WITH_NAMESPACE(CameraComponent, DAVA::);
REGISTER_CLASS_WITH_NAMESPACE(StaticOcclusionComponent, DAVA::);
REGISTER_CLASS_WITH_NAMESPACE(StaticOcclusionDataComponent, DAVA::);
REGISTER_CLASS_WITH_NAMESPACE(PathComponent, DAVA::);
REGISTER_CLASS_WITH_NAMESPACE(WASDControllerComponent, DAVA::);
REGISTER_CLASS_WITH_NAMESPACE(RotationControllerComponent, DAVA::);
REGISTER_CLASS_WITH_NAMESPACE(SnapToLandscapeControllerComponent, DAVA::);
REGISTER_CLASS_WITH_NAMESPACE(GeoDecalComponent, DAVA::);
REGISTER_CLASS_WITH_NAMESPACE(SlotComponent, DAVA::);
REGISTER_CLASS_WITH_NAMESPACE(TextComponent, DAVA::);

#if defined(__DAVAENGINE_PHYSICS_ENABLED__)
REGISTER_CLASS_WITH_NAMESPACE(StaticBodyComponent, DAVA::);
REGISTER_CLASS_WITH_NAMESPACE(DynamicBodyComponent, DAVA::);
REGISTER_CLASS_WITH_NAMESPACE(BoxShapeComponent, DAVA::);
REGISTER_CLASS_WITH_NAMESPACE(CapsuleShapeComponent, DAVA::);
REGISTER_CLASS_WITH_NAMESPACE(SphereShapeComponent, DAVA::);
REGISTER_CLASS_WITH_NAMESPACE(PlaneShapeComponent, DAVA::);
REGISTER_CLASS_WITH_NAMESPACE(ConvexHullShapeComponent, DAVA::);
REGISTER_CLASS_WITH_NAMESPACE(MeshShapeComponent, DAVA::);
REGISTER_CLASS_WITH_NAMESPACE(HeightFieldShapeComponent, DAVA::);
REGISTER_CLASS_WITH_NAMESPACE(BoxCharacterControllerComponent, DAVA::);
REGISTER_CLASS_WITH_NAMESPACE(CapsuleCharacterControllerComponent, DAVA::);
REGISTER_CLASS_WITH_NAMESPACE(WASDPhysicsControllerComponent, DAVA::);
REGISTER_CLASS_WITH_NAMESPACE(VehicleCarComponent, DAVA::);
REGISTER_CLASS_WITH_NAMESPACE(VehicleTankComponent, DAVA::);
REGISTER_CLASS_WITH_NAMESPACE(VehicleChassisComponent, DAVA::);
REGISTER_CLASS_WITH_NAMESPACE(VehicleWheelComponent, DAVA::);
#endif

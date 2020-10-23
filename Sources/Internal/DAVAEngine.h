#pragma once

#include "DAVAConfig.h"
#include "MemoryManager/MemoryProfiler.h"

#include "Base/BaseTypes.h"
#include "Base/BaseMath.h"
#include "Utils/StringFormat.h"

#include "DLC/DLC.h"
#include "DLC/Patcher/PatchFile.h"
#include "DLC/Downloader/DownloadManager.h"

#include "Logger/Logger.h"

#include "Platform/DeviceInfo.h"

#include "Time/DateTime.h"
#include "Time/SystemTimer.h"

// system stuff
#include "Utils/Utils.h"
#include "Utils/UTF8Utils.h"
#include "Utils/MD5.h"
#include "Base/Message.h"
#include "Base/BaseObject.h"
#include "Debug/DVAssert.h"
#include "Base/Singleton.h"
#include "Utils/StringFormat.h"
#include "UI/ScrollHelper.h"
#include "Debug/Replay.h"
#include "Utils/Random.h"

#include "Base/ObjectFactory.h"
#include "Base/FixedSizePoolAllocator.h"

// ptrs
#include "Base/RefPtr.h"
#include "Base/ScopedPtr.h"

// threads
#include "Concurrency/Concurrency.h"

// Accelerometer
#include "Input/Accelerometer.h"

#include "Input/InputSystem.h"
#include "Input/Gamepad.h"

// Localization
#include "FileSystem/LocalizationSystem.h"

// Image formats stuff (PNG & JPG & other formats)
#include "Render/Image/LibPngHelper.h"
#include "Render/Image/Image.h"
#include "Render/Image/ImageSystem.h"
#include "Render/Image/LibDdsHelper.h"

// Files & Serialization
#include "FileSystem/FileSystem.h"
#include "FileSystem/File.h"
#include "FileSystem/FileList.h"
#include "FileSystem/VariantType.h"
#include "FileSystem/KeyedArchiver.h"
#include "FileSystem/KeyedUnarchiver.h"
#include "FileSystem/KeyedArchive.h"

#include "FileSystem/XMLParser.h"
#include "FileSystem/YamlNode.h"
#include "FileSystem/YamlParser.h"
#include "FileSystem/YamlEmitter.h"
#include "FileSystem/Parser.h"
#include "FileSystem/FilePath.h"

// Collisions
#include "Collision/Collisions.h"

// Animation manager
#include "Animation/Interpolation.h"
#include "Animation/AnimatedObject.h"
#include "Animation/Animation.h"
#include "Animation/AnimationManager.h"
#include "Animation/LinearAnimation.h"
#include "Animation/BezierSplineAnimation.h"

// 2D Graphics
#include "Render/2D/Sprite.h"
#include "Render/GPUFamilyDescriptor.h"
#include "Render/TextureDescriptor.h"
#include "Render/Texture.h"
#include "Render/Shader.h"
#include "Render/ShaderCache.h"
#include "Core/DisplayMode.h"
#include "Render/RenderHelper.h"
#include "Render/MipmapReplacer.h"
#include "Render/2D/Systems/VirtualCoordinatesSystem.h"
#include "Render/2D/Systems/RenderSystem2D.h"

// Fonts
#include "Render/2D/Font.h"
#include "Render/2D/GraphicFont.h"
#include "Render/2D/FTFont.h"
#include "Render/2D/FontManager.h"
#include "Render/2D/TextBlock.h"

// UI
#include "UI/UIControl.h"
#include "UI/UIControlSystem.h"
#include "UI/UIPackage.h"
#include "UI/UIPackagesCache.h"
#include "UI/UIPackageLoader.h"
#include "UI/DefaultUIPackageBuilder.h"
#include "UI/UIEvent.h"
#include "UI/UIButton.h"
#include "UI/UIStaticText.h"
#include "UI/UIControlBackground.h"
#include "UI/UIScreen.h"
#include "UI/UILoadingScreen.h"
#include "UI/UIList.h"
#include "UI/UIListCell.h"
#include "UI/UITextField.h"
#include "UI/UISlider.h"
#include "UI/UIScrollBar.h"
#include "UI/UIJoypad.h"
#include "UI/UIFileSystemDialog.h"
#include "UI/UIWebView.h"
#include "UI/UIScrollView.h"
#include "UI/UI3DView.h"
#include "UI/UISpinner.h"
#include "UI/VectorSpinnerAdapter.h"
#include "UI/UISwitch.h"
#include "UI/UIParticles.h"
#include "UI/UIMovieView.h"

#include "UI/UIYamlLoader.h"

#include "UI/UIScreenTransition.h"
#include "UI/UIMoveInTransition.h"
#include "UI/UIFadeTransition.h"
#include "UI/UIHoleTransition.h"

#include "UI/UIScreenManager.h"

#include "UI/UIScrollViewContainer.h"
#include "UI/UIControlHelpers.h"
#include "UI/UIScreenshoter.h"

// Game object manager / 2D Scene
#include "Scene2D/GameObject.h"
#include "Scene2D/GameObjectManager.h"
#include "Collision/CollisionObject2.h"

// Sound & Music
#include "Sound/SoundEvent.h"
#include "Sound/SoundSystem.h"

// Particle System
#include "Particles/ParticleEmitter.h"
#include "Particles/ParticleLayer.h"
#include "Particles/Particle.h"

// 3D core classes
#include "Scene3D/SceneFileV2.h"
#include "Scene3D/SceneFile/SerializationContext.h"

#include "Render/3D/StaticMesh.h"
#include "Render/3D/PolygonGroup.h"
#include "Render/3D/EdgeAdjacency.h"
#include "Render/3D/MeshUtils.h"

#include "Render/RenderHelper.h"
#include "Render/DynamicBufferAllocator.h"

#include "Render/Material/NMaterialNames.h"

// 3D scene management
#include "Scene3D/Scene.h"
#include "Scene3D/Entity.h"
#include "Render/Highlevel/RenderPass.h"
#include "Render/Highlevel/RenderPassNames.h"
#include "Render/Highlevel/Landscape.h"
#include "Render/Highlevel/Heightmap.h"
#include "Render/Highlevel/Light.h"
#include "Render/Highlevel/Mesh.h"
#include "Render/Highlevel/SkinnedMesh.h"
#include "Render/Highlevel/SpriteObject.h"
#include "Render/Highlevel/RenderObject.h"
#include "Render/Highlevel/SpeedTreeObject.h"
#include "Render/Highlevel/BillboardRenderObject.h"
#include "Render/Highlevel/Vegetation/VegetationRenderObject.h"

#include "Scene3D/AnimationData.h"
#include "Scene3D/Systems/EventSystem.h"
#include "Scene3D/Systems/GlobalEventSystem.h"
#include "Scene3D/Systems/SpeedTreeUpdateSystem.h"
#include "Scene3D/Systems/QualitySettingsSystem.h"
#include "Scene3D/Systems/FoliageSystem.h"
#include "Scene3D/Systems/ParticleEffectSystem.h"
#include "Scene3D/Lod/LodSystem.h"
#include "Scene3D/Lod/LodComponent.h"

//Components
#include "Scene3D/Components/ComponentHelpers.h"
#include "Scene3D/Components/AnimationComponent.h"
#include "Scene3D/Components/BulletComponent.h"
#include "Scene3D/Components/CameraComponent.h"
#include "Scene3D/Components/DebugRenderComponent.h"
#include "Scene3D/Components/LightComponent.h"
#include "Scene3D/Components/ParticleEffectComponent.h"
#include "Scene3D/Components/RenderComponent.h"
#include "Scene3D/Components/TransformComponent.h"
#include "Scene3D/Components/UpdatableComponent.h"
#include "Scene3D/Components/SwitchComponent.h"
#include "Scene3D/Components/UserComponent.h"
#include "Scene3D/Components/ActionComponent.h"
#include "Scene3D/Components/SoundComponent.h"
#include "Scene3D/Components/CustomPropertiesComponent.h"
#include "Scene3D/Components/StaticOcclusionComponent.h"
#include "Scene3D/Components/QualitySettingsComponent.h"
#include "Scene3D/Components/SpeedTreeComponent.h"
#include "Scene3D/Components/WindComponent.h"
#include "Scene3D/Components/WaveComponent.h"
#include "Scene3D/Components/SlotComponent.h"
#include "Scene3D/Components/TextComponent.h"
#include "Scene3D/Components/Waypoint/PathComponent.h"
#include "Scene3D/Components/GeoDecalComponent.h"
#include "Scene3D/Components/Controller/WASDControllerComponent.h"
#include "Scene3D/Components/Controller/RotationControllerComponent.h"
#include "Scene3D/Components/Controller/SnapToLandscapeControllerComponent.h"

#include "Job/JobManager.h"

// Notifications
#include "Notification/LocalNotification.h"
#include "Notification/LocalNotificationText.h"
#include "Notification/LocalNotificationProgress.h"
#include "Notification/LocalNotificationDelayed.h"
#include "Notification/LocalNotificationController.h"

#include "Classes/SceneTree/Private/CreateEntitySupportDefault.h"
#include "Classes/Qt/Tools/AddSwitchEntityDialog/AddSwitchEntityDialog.h"

#include <REPlatform/DataNodes/ProjectManagerData.h>
#include <REPlatform/Commands/EntityAddCommand.h>
#include <REPlatform/Global/SceneTree/CreateEntitySupport.h>
#include <REPlatform/Global/StringConstants.h>

#include <TArc/Core/ContextAccessor.h>
#include <TArc/WindowSubSystem/UI.h>
#include <TArc/Utils/QtConnections.h>
#include <TArc/Utils/Utils.h>

#include <Base/FastName.h>
#include <Base/RefPtr.h>
#include <Engine/Engine.h>
#include <Engine/EngineContext.h>
#include <Functional/Function.h>
#include <Reflection/ReflectedTypeDB.h>
#include <Reflection/ReflectionRegistrator.h>
#include <Render/2D/Systems/VirtualCoordinatesSystem.h>
#include <Render/Highlevel/Landscape.h>
#include <Render/Highlevel/SpriteObject.h>
#include <Render/Highlevel/Vegetation/VegetationRenderObject.h>
#include <Scene3D/Components/CameraComponent.h>
#include <Scene3D/Components/ComponentHelpers.h>
#include <Scene3D/Components/Controller/RotationControllerComponent.h>
#include <Scene3D/Components/Controller/WASDControllerComponent.h>
#include <Scene3D/Components/LightComponent.h>
#include <Scene3D/Components/ParticleEffectComponent.h>
#include <Scene3D/Components/RenderComponent.h>
#include <Scene3D/Components/TransformComponent.h>
#include <Scene3D/Components/Waypoint/PathComponent.h>
#include <Scene3D/Components/WindComponent.h>
#include <Scene3D/Entity.h>
#include <Scene3D/Lod/LodComponent.h>
#include <UI/UIControlSystem.h>

DAVA_VIRTUAL_TEMPLATE_REFLECTION_IMPL(SimpleCreatorHelper)
{
    DAVA::ReflectionRegistrator<SimpleCreatorHelper<T>>::Begin()
    .End();
}

namespace CreateEntitySupportDetails
{
using namespace DAVA;
class EmptyEntityCreator : public SimpleCreatorHelper<EmptyEntityCreator>
{
    using TBase = SimpleCreatorHelper<EmptyEntityCreator>;

public:
    static RefPtr<Entity> CreateEntity()
    {
        RefPtr<Entity> e(new Entity());
        e->SetName(FastName(ResourceEditor::ENTITY_NAME));
        return e;
    }

    EmptyEntityCreator()
        : TBase(eMenuPointOrder::EMPTY_ENTITY, SharedIcon(":/QtIcons/node.png"), QStringLiteral("Empty Entity"))
    {
    }

    DAVA_VIRTUAL_REFLECTION_IN_PLACE(EmptyEntityCreator, TBase)
    {
        ReflectionRegistrator<EmptyEntityCreator>::Begin()
        .ConstructorByPointer()
        .End();
    }
};

class LightEntityCreator : public SimpleCreatorHelper<LightEntityCreator>
{
    using TBase = SimpleCreatorHelper<LightEntityCreator>;

public:
    static RefPtr<Entity> CreateEntity()
    {
        RefPtr<Entity> sceneNode(new Entity());
        sceneNode->AddComponent(new LightComponent(ScopedPtr<Light>(new Light)));
        sceneNode->SetName(FastName(ResourceEditor::LIGHT_NODE_NAME));
        return sceneNode;
    }

    LightEntityCreator()
        : TBase(eMenuPointOrder::LIGHT_ENTITY, SharedIcon(":/QtIcons/light.png"), QStringLiteral("Light"))
    {
    }

    DAVA_VIRTUAL_REFLECTION_IN_PLACE(LightEntityCreator, TBase)
    {
        ReflectionRegistrator<LightEntityCreator>::Begin()
        .ConstructorByPointer()
        .End();
    }
};

class CloneCurrentCamera : public EntityCreator
{
public:
    CloneCurrentCamera()
        : EntityCreator(SharedIcon(":/QtIcons/camera.png"), QStringLiteral("Clone Current Camera"))
    {
    }

    eMenuPointOrder GetOrder() const override
    {
        return eMenuPointOrder::CAMERA_ENTITY;
    }

protected:
    void StartEntityCreationImpl() override
    {
        Camera* currentCamera = scene->GetCurrentCamera();
        DVASSERT(currentCamera != nullptr);

        ScopedPtr<Camera> camera(static_cast<Camera*>(currentCamera->Clone()));

        ScopedPtr<DAVA::Entity> sceneNode(new Entity());
        sceneNode->AddComponent(new CameraComponent(camera));
        sceneNode->AddComponent(new WASDControllerComponent());
        sceneNode->AddComponent(new RotationControllerComponent());
        sceneNode->SetName(FastName(ResourceEditor::CAMERA_NODE_NAME));

        AddEntity(sceneNode);
        FinishCreation();
    }
};

class CameraEntityCreatorGroup : public EntityCreatorsGroup
{
public:
    CameraEntityCreatorGroup()
        : EntityCreatorsGroup(DAVA::SharedIcon(":/QtIcons/camera.png"), QStringLiteral("Camera"))
    {
        creatorsGroup.push_back(std::make_unique<SimpleEntityCreator>(eMenuPointOrder::CAMERA_ENTITY,
                                                                      SharedIcon(":/QtIcons/camera.png"), QStringLiteral("Camera"), DAVA::MakeFunction(&CameraEntityCreatorGroup::CreateCamera)));

        creatorsGroup.push_back(std::make_unique<SimpleEntityCreator>(eMenuPointOrder::CAMERA_ENTITY,
                                                                      SharedIcon(":/QtIcons/camera.png"), QStringLiteral("Camera 2D"), DAVA::MakeFunction(&CameraEntityCreatorGroup::CreateCamera2D)));

        creatorsGroup.push_back(std::make_unique<CloneCurrentCamera>());
    }

    eMenuPointOrder GetOrder() const override
    {
        return eMenuPointOrder::CAMERA_ENTITY;
    }

private:
    static RefPtr<Entity> CreateCamera()
    {
        RefPtr<Entity> e(new Entity());
        RefPtr<Camera> camera(new Camera());

        camera->SetUp(Vector3(0.0f, 0.0f, 1.0f));
        camera->SetPosition(Vector3(0.0f, 0.0f, 0.0f));
        camera->SetTarget(Vector3(1.0f, 0.0f, 0.0f));
        camera->SetupPerspective(70.0f, 320.0f / 480.0f, 1.0f, 5000.0f);
        camera->SetAspect(1.0f);
        camera->RebuildCameraFromValues();

        e->AddComponent(new CameraComponent(camera.Get()));
        e->AddComponent(new WASDControllerComponent());
        e->AddComponent(new RotationControllerComponent());

        e->SetName(FastName(ResourceEditor::CAMERA_NODE_NAME));
        return e;
    }

    static RefPtr<Entity> CreateCamera2D()
    {
        RefPtr<Entity> e(new Entity());
        RefPtr<Camera> camera(new Camera());

        float32 w = GetEngineContext()->uiControlSystem->vcs->GetFullScreenVirtualRect().dx;
        float32 h = GetEngineContext()->uiControlSystem->vcs->GetFullScreenVirtualRect().dy;
        float32 aspect = w / h;
        camera->SetupOrtho(w, aspect, 1, 1000);
        camera->SetPosition(Vector3(0, 0, -10000));
        camera->SetZFar(10000);
        camera->SetTarget(Vector3(0, 0, 0));
        camera->SetUp(Vector3(0, -1, 0));
        camera->RebuildCameraFromValues();

        e->AddComponent(new CameraComponent(camera.Get()));
        e->SetName("Camera 2D");
        return e;
    }

    DAVA_VIRTUAL_REFLECTION_IN_PLACE(CameraEntityCreatorGroup, EntityCreatorsGroup)
    {
        ReflectionRegistrator<CameraEntityCreatorGroup>::Begin()
        .ConstructorByPointer()
        .End();
    }
};

class ParticleEffectEntityCreator : public SimpleCreatorHelper<ParticleEffectEntityCreator>
{
    using TBase = SimpleCreatorHelper<ParticleEffectEntityCreator>;

public:
    static RefPtr<Entity> CreateEntity()
    {
        RefPtr<Entity> sceneNode(new Entity());
        sceneNode->AddComponent(new ParticleEffectComponent());
        sceneNode->AddComponent(new LodComponent());
        sceneNode->SetName(FastName(ResourceEditor::PARTICLE_EFFECT_NODE_NAME));

        return sceneNode;
    }

    ParticleEffectEntityCreator()
        : TBase(eMenuPointOrder::PARTICLE_EFFECT_ENTITY, SharedIcon(":/QtIcons/effect.png"), QStringLiteral("Particle Effect Node"))
    {
    }

    DAVA_VIRTUAL_REFLECTION_IN_PLACE(ParticleEffectEntityCreator, TBase)
    {
        ReflectionRegistrator<ParticleEffectEntityCreator>::Begin()
        .ConstructorByPointer()
        .End();
    }
};

class LandscapeEntityCreator : public SimpleCreatorHelper<LandscapeEntityCreator>
{
    using TBase = SimpleCreatorHelper<LandscapeEntityCreator>;

public:
    static RefPtr<Entity> CreateEntity()
    {
        RefPtr<Entity> entityToProcess(new Entity());
        entityToProcess->SetName(FastName(ResourceEditor::LANDSCAPE_NODE_NAME));
        entityToProcess->SetLocked(true);

        RefPtr<Landscape> newLandscape(new Landscape());

        RenderComponent* component = new RenderComponent();
        component->SetRenderObject(newLandscape.Get());
        entityToProcess->AddComponent(component);

        AABBox3 bboxForLandscape;
        float32 defaultLandscapeSize = 600.0f;
        float32 defaultLandscapeHeight = 50.0f;

        bboxForLandscape.AddPoint(Vector3(-defaultLandscapeSize / 2.f, -defaultLandscapeSize / 2.f, 0.f));
        bboxForLandscape.AddPoint(Vector3(defaultLandscapeSize / 2.f, defaultLandscapeSize / 2.f, defaultLandscapeHeight));
        newLandscape->BuildLandscapeFromHeightmapImage("", bboxForLandscape);

        return entityToProcess;
    }

    LandscapeEntityCreator()
        : TBase(eMenuPointOrder::LANDSCAPE_ENTITY, SharedIcon(":/QtIcons/land_settings.png"), QStringLiteral("Landscape"))
    {
    }

    DAVA_VIRTUAL_REFLECTION_IN_PLACE(LandscapeEntityCreator, TBase)
    {
        ReflectionRegistrator<LandscapeEntityCreator>::Begin()
        .ConstructorByPointer()
        .End();
    }
};

class WindEntityCreator : public SimpleCreatorHelper<WindEntityCreator>
{
    using TBase = SimpleCreatorHelper<WindEntityCreator>;

public:
    static RefPtr<Entity> CreateEntity()
    {
        RefPtr<Entity> windEntity(new Entity());
        windEntity->SetName(FastName(ResourceEditor::WIND_NODE_NAME));

        TransformComponent* tc = windEntity->GetComponent<TransformComponent>();
        tc->SetLocalTranslation(Vector3(0.f, 0.f, 20.f));

        windEntity->AddComponent(new DAVA::WindComponent());

        return windEntity;
    }

    WindEntityCreator()
        : TBase(eMenuPointOrder::WIND_ENTITY, SharedIcon(":/QtIcons/wind.png"), QStringLiteral("Wind"))
    {
    }

    DAVA_VIRTUAL_REFLECTION_IN_PLACE(WindEntityCreator, TBase)
    {
        ReflectionRegistrator<WindEntityCreator>::Begin()
        .ConstructorByPointer()
        .End();
    }
};

class VegetationEntityCreator : public SimpleCreatorHelper<VegetationEntityCreator>
{
    using TBase = SimpleCreatorHelper<VegetationEntityCreator>;

public:
    static RefPtr<Entity> CreateEntity()
    {
        RefPtr<VegetationRenderObject> vro(new VegetationRenderObject());
        RenderComponent* rc = new RenderComponent();
        rc->SetRenderObject(vro.Get());

        RefPtr<Entity> vegetationNode(new Entity());
        vegetationNode->AddComponent(rc);
        vegetationNode->SetName(FastName(ResourceEditor::VEGETATION_NODE_NAME));
        vegetationNode->SetLocked(true);

        return vegetationNode;
    }

    VegetationEntityCreator()
        : TBase(eMenuPointOrder::VEGETATION_ENTITY, SharedIcon(":/QtIcons/grass.png"), QStringLiteral("Vegetation"))
    {
    }

    DAVA_VIRTUAL_REFLECTION_IN_PLACE(VegetationEntityCreator, TBase)
    {
        ReflectionRegistrator<VegetationEntityCreator>::Begin()
        .ConstructorByPointer()
        .End();
    }
};

class PathEntityCreator : public SimpleCreatorHelper<PathEntityCreator>
{
    using TBase = SimpleCreatorHelper<PathEntityCreator>;

public:
    static RefPtr<Entity> CreateEntity()
    {
        RefPtr<Entity> pathEntity(new Entity());
        pathEntity->SetName(FastName(ResourceEditor::PATH_NODE_NAME));
        PathComponent* pc = new PathComponent();

        pathEntity->AddComponent(pc);

        return pathEntity;
    }

    PathEntityCreator()
        : TBase(eMenuPointOrder::PATH_ENTITY, SharedIcon(":/QtIcons/path.png"), QStringLiteral("Path"))
    {
    }

    DAVA_VIRTUAL_REFLECTION_IN_PLACE(PathEntityCreator, TBase)
    {
        ReflectionRegistrator<PathEntityCreator>::Begin()
        .ConstructorByPointer()
        .End();
    }
};

class EditorSpriteEntityCreator : public EntityCreator
{
public:
    void StartEntityCreationImpl() override
    {
        DVASSERT(accessor != nullptr);
        DVASSERT(ui != nullptr);
        ProjectManagerData* data = accessor->GetGlobalContext()->GetData<ProjectManagerData>();
        DVASSERT(data != nullptr);
        DAVA::FilePath projectPath = data->GetParticlesGfxPath();

        FileDialogParams params;
        params.dir = QString::fromStdString(projectPath.GetAbsolutePathname());
        params.title = QStringLiteral("Open sprite");
        params.filters = QStringLiteral("Sprite File (*.psd)");
        QString filePath = ui->GetOpenFileName(DAVA::mainWindowKey, params);

        if (filePath.isEmpty())
            return;

        filePath = filePath.replace("/DataSource/", "/Data/");
        filePath.remove(filePath.size() - 4, 4);

        RefPtr<Sprite> sprite(Sprite::Create(filePath.toStdString()));
        if (!sprite)
            return;

        RefPtr<Entity> sceneNode(new Entity());
        sceneNode->SetName(FastName(ResourceEditor::EDITOR_SPRITE));

        RefPtr<SpriteObject> spriteObject(new SpriteObject(sprite.Get(), 0, Vector2(1, 1), Vector2(0.5f * sprite->GetWidth(), 0.5f * sprite->GetHeight())));
        spriteObject->AddFlag(RenderObject::ALWAYS_CLIPPING_VISIBLE);
        sceneNode->AddComponent(new RenderComponent(spriteObject.Get()));
        Matrix4 m = Matrix4(1, 0, 0, 0,
                            0, 1, 0, 0,
                            0, 0, -1, 0,
                            0, 0, 0, 1);

        TransformComponent* tc = sceneNode->GetComponent<TransformComponent>();
        tc->SetLocalMatrix(m);

        AddEntity(sceneNode.Get());
        FinishCreation();
    }

    EditorSpriteEntityCreator()
        : EntityCreator(SharedIcon(":/QtIcons/map.png"), QStringLiteral("Editor Sprite"))
    {
    }

    eMenuPointOrder GetOrder() const override
    {
        return eMenuPointOrder::EDITOR_SPRITE;
    }

    DAVA_VIRTUAL_REFLECTION_IN_PLACE(EditorSpriteEntityCreator, EntityCreator)
    {
        ReflectionRegistrator<EditorSpriteEntityCreator>::Begin()
        .ConstructorByPointer()
        .End();
    }
};

class SwitchEntityCreator : public EntityCreator
{
public:
    void StartEntityCreationImpl() override
    {
        DVASSERT(dlg == nullptr);
        dlg = new AddSwitchEntityDialog();
        dlg->setWindowFlags(dlg->windowFlags() | Qt::WindowStaysOnTopHint);
        connections.AddConnection(dlg, &QDialog::finished, MakeFunction(this, &SwitchEntityCreator::OnDialogFinished));
        ui->ShowDialog(DAVA::mainWindowKey, dlg);
    }

    SwitchEntityCreator()
        : EntityCreator(SharedIcon(":/QtIcons/switch.png"), QStringLiteral("Switch"))
    {
    }

    void Cancel() override
    {
        DVASSERT(dlg != nullptr);
        dlg->deleteLater();
    }

    eMenuPointOrder GetOrder() const override
    {
        return eMenuPointOrder::SWITCH_ENTITY;
    }

private:
    void OnDialogFinished(int resultCode)
    {
        FinishCreation();
        Cancel();
    }

private:
    QPointer<AddSwitchEntityDialog> dlg;
    QtConnections connections;

    DAVA_VIRTUAL_REFLECTION_IN_PLACE(SwitchEntityCreator, EntityCreator)
    {
        ReflectionRegistrator<SwitchEntityCreator>::Begin()
        .ConstructorByPointer()
        .End();
    }
};

} // namespace CreateEntitySupportDetails

std::unique_ptr<DAVA::BaseEntityCreator> CreateEntityCreationTree()
{
    using namespace CreateEntitySupportDetails;
    DAVA_REFLECTION_REGISTER_PERMANENT_NAME(EmptyEntityCreator);
    DAVA_REFLECTION_REGISTER_PERMANENT_NAME(LightEntityCreator);
    DAVA_REFLECTION_REGISTER_PERMANENT_NAME(CameraEntityCreatorGroup);
    DAVA_REFLECTION_REGISTER_PERMANENT_NAME(ParticleEffectEntityCreator);
    DAVA_REFLECTION_REGISTER_PERMANENT_NAME(LandscapeEntityCreator);
    DAVA_REFLECTION_REGISTER_PERMANENT_NAME(WindEntityCreator);
    DAVA_REFLECTION_REGISTER_PERMANENT_NAME(VegetationEntityCreator);
    DAVA_REFLECTION_REGISTER_PERMANENT_NAME(PathEntityCreator);
    DAVA_REFLECTION_REGISTER_PERMANENT_NAME(EditorSpriteEntityCreator);
    DAVA_REFLECTION_REGISTER_PERMANENT_NAME(SwitchEntityCreator);

    const DAVA::ReflectedType* baseEntityCreatorType = DAVA::ReflectedTypeDB::Get<BaseEntityCreator>();

    DAVA::Vector<BaseEntityCreator*> createdObjects;
    DAVA::Function<void(const DAVA::ReflectedType*)> functor = [&createdObjects, &functor](const DAVA::ReflectedType* baseType) {
        const DAVA::TypeInheritance* inheritance = baseType->GetType()->GetInheritance();
        if (inheritance == nullptr)
        {
            return;
        }

        const DAVA::Vector<DAVA::TypeInheritance::Info>& derivedTypes = inheritance->GetDerivedTypes();
        for (const DAVA::TypeInheritance::Info& derived : derivedTypes)
        {
            const DAVA::ReflectedType* derivedType = DAVA::ReflectedTypeDB::GetByType(derived.type);
            if (derivedType != nullptr)
            {
                const AnyFn* ctr = derivedType->GetCtor(derivedType->GetType()->Pointer());
                if (ctr != nullptr)
                {
                    createdObjects.push_back(derivedType->CreateObject(ReflectedType::CreatePolicy::ByPointer).Cast<BaseEntityCreator*>());
                }

                functor(derivedType);
            }
        }
    };

    functor(baseEntityCreatorType);

    std::sort(createdObjects.begin(), createdObjects.end(), [](const BaseEntityCreator* left, const BaseEntityCreator* right) {
        return left->GetOrder() < right->GetOrder();
    });

    EntityCreatorsGroup* rootGroup = new EntityCreatorsGroup(QIcon(), QString());
    rootGroup->creatorsGroup.reserve(createdObjects.size());
    for (BaseEntityCreator* creator : createdObjects)
    {
        rootGroup->creatorsGroup.push_back(std::unique_ptr<BaseEntityCreator>(creator));
    }

    return std::unique_ptr<BaseEntityCreator>(rootGroup);
}

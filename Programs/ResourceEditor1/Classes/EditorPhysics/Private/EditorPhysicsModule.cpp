#include "Classes/EditorPhysics/EditorPhysicsModule.h"
#include "Classes/EditorPhysics/Private/EditorPhysicsSystem.h"
#include "Classes/EditorPhysics/Private/EditorPhysicsData.h"
#include "Classes/EditorPhysics/Private/PhysicsWidget.h"

#include "Classes/Commands2/EntityAddCommand.h"
#include "Classes/Interfaces/PropertyPanelInterface.h"
#include "Classes/Project/ProjectManagerData.h"
#include "Classes/Qt/Scene/SceneEditor2.h"
#include "Classes/SceneManager/SceneData.h"
#include "Classes/SceneTree/CreateEntitySupport.h"

#include <TArc/WindowSubSystem/ActionUtils.h>
#include <TArc/WindowSubSystem/UI.h>
#include <TArc/Utils/ModuleCollection.h>
#include <TArc/Utils/ReflectedPairsVector.h>
#include <TArc/Qt/QtString.h>
#include <TArc/Controls/PropertyPanel/BaseComponentValue.h>
#include <TArc/Controls/PropertyPanel/PropertyModelExtensions.h>
#include <TArc/Controls/ComboBox.h>

#include <Physics/PhysicsModule.h>
#include <Physics/VehicleWheelComponent.h>
#include <Physics/VehicleChassisComponent.h>
#include <Physics/VehicleCarComponent.h>
#include <Physics/VehicleTankComponent.h>
#include <Physics/ConvexHullShapeComponent.h>
#include <Physics/DynamicBodyComponent.h>
#include <Physics/BoxShapeComponent.h>

#include <Scene3D/Scene.h>
#include <Scene3D/Components/RenderComponent.h>
#include <Render/Highlevel/RenderObject.h>
#include <Reflection/ReflectionRegistrator.h>
#include <Reflection/ReflectedTypeDB.h>
#include <Base/Type.h>

#include <QAction>
#include <QList>

namespace EditorPhysicsDetail
{
using namespace DAVA;
using namespace DAVA::TArc;
class PhysicsMaterialComponentValue : public BaseComponentValue
{
public:
    PhysicsMaterialComponentValue()
    {
        PhysicsModule* module = GetEngineContext()->moduleManager->GetModule<PhysicsModule>();
        DVASSERT(module != nullptr);

        Vector<FastName> materialNames = module->GetMaterialNames();

        std::sort(materialNames.begin(), materialNames.end(), [](const FastName& name1, const FastName& name2) {
            DVASSERT(name1.IsValid() == true);
            DVASSERT(name2.IsValid() == true);
            return strcmp(name1.c_str(), name2.c_str()) < 0;
        });

        materials.values.emplace_back(FastName(), String("Default material"));
        for (const FastName& name : materialNames)
        {
            materials.values.emplace_back(name, name.c_str());
        }
    }

    Any GetMultipleValue() const override
    {
        return Any();
    }

    bool IsValidValueToSet(const Any& newValue, const Any& currentValue) const override
    {
        if (currentValue.IsEmpty())
        {
            return true;
        }
        return newValue != currentValue;
    }

    ControlProxy* CreateEditorWidget(QWidget* parent, const Reflection& model, DataWrappersProcessor* wrappersProcessor) override
    {
        ComboBox::Params params(GetAccessor(), GetUI(), GetWindowKey());
        params.fields[ComboBox::Fields::Enumerator] = "materials";
        params.fields[ComboBox::Fields::Value] = "currentMaterial";
        params.fields[ComboBox::Fields::MultipleValueText] = "unregisteredMaterial";
        return new ComboBox(params, wrappersProcessor, model, parent);
    }

private:
    Any GetCurrentMaterial()
    {
        return GetValue();
    }

    void SetCurrentMaterial(const Any& materialName)
    {
        SetValue(materialName);
    }

    ReflectedPairsVector<FastName, String> materials;

    DAVA_VIRTUAL_REFLECTION_IN_PLACE(PhysicsMaterialComponentValue, BaseComponentValue)
    {
        DAVA::ReflectionRegistrator<PhysicsMaterialComponentValue>::Begin()
        .Field("materials", &PhysicsMaterialComponentValue::materials)
        .Field("currentMaterial", &PhysicsMaterialComponentValue::GetCurrentMaterial, &PhysicsMaterialComponentValue::SetCurrentMaterial)
        .Field("unregisteredMaterial", [](PhysicsMaterialComponentValue*) -> String { return "Unregistered material"; }, nullptr)
        .End();
    }
};

class PhysicsEditorCreator : public EditorComponentExtension
{
public:
    std::unique_ptr<DAVA::TArc::BaseComponentValue> GetEditor(const std::shared_ptr<const DAVA::TArc::PropertyNode>& node) const override
    {
        std::shared_ptr<DAVA::TArc::PropertyNode> parentNode = node->parent.lock();
        if (parentNode != nullptr)
        {
            const DAVA::Type* valueType = node->cachedValue.GetType();
            if (node->propertyType == DAVA::TArc::PropertyNode::RealProperty &&
                valueType == DAVA::Type::Instance<DAVA::FastName>() &&
                parentNode->cachedValue.CanCast<DAVA::CollisionShapeComponent*>() &&
                node->field.key.Cast<DAVA::String>() == "material")
            {
                return std::make_unique<PhysicsMaterialComponentValue>();
            }
        }

        return EditorComponentExtension::GetEditor(node);
    }
};

class EditorPhysicsGlobalData : public DAVA::TArc::DataNode
{
public:
    std::shared_ptr<DAVA::TArc::EditorComponentExtension> physicsExtension;

private:
    DAVA_VIRTUAL_REFLECTION_IN_PLACE(EditorPhysicsGlobalData, DataNode)
    {
        ReflectionRegistrator<EditorPhysicsGlobalData>::Begin()
        .End();
    }
};

DAVA::RenderObject* CreateVehicleWheelRenderObject(DAVA::float32 radius, DAVA::float32 width)
{
    using namespace DAVA;

    // Generate wheel mesh triangles

    const size_t segments = 36;

    Array<Vector3, segments * 2> vertices;
    for (size_t i = 0; i < segments; ++i)
    {
        const float32 angle = i * PI * 2.0f / segments;
        const float32 x = radius * std::cos(angle);
        const float32 z = radius * std::sin(angle);
        vertices[2 * i + 0] = Vector3(x, -width / 2.0f, z);
        vertices[2 * i + 1] = Vector3(x, +width / 2.0f, z);
    }

    // Sectors * 2 triangles
    Array<uint16, segments * 2 * 3> indices;
    for (uint16 i = 0; i < segments; ++i)
    {
        if (i == segments - 1)
        {
            // Loop with the first indices

            indices[i * 6 + 0] = i * 2 + 0;
            indices[i * 6 + 1] = 0;
            indices[i * 6 + 2] = i * 2 + 1;

            indices[i * 6 + 3] = i * 2 + 1;
            indices[i * 6 + 4] = 0;
            indices[i * 6 + 5] = 1;
        }
        else
        {
            indices[i * 6 + 0] = i * 2 + 0;
            indices[i * 6 + 1] = i * 2 + 2;
            indices[i * 6 + 2] = i * 2 + 1;

            indices[i * 6 + 3] = i * 2 + 1;
            indices[i * 6 + 4] = i * 2 + 2;
            indices[i * 6 + 5] = i * 2 + 3;
        }
    }

    // Create render object

    RefPtr<NMaterial> material(new NMaterial());
    material->SetMaterialName(FastName("VehicleWheelMaterial"));
    material->SetFXName(NMaterialName::DEBUG_DRAW_WIREFRAME);
    material->AddProperty(FastName("color"), Color::White.color, rhi::ShaderProp::TYPE_FLOAT4);

    RefPtr<PolygonGroup> group(new PolygonGroup());
    group->SetPrimitiveType(rhi::PRIMITIVE_TRIANGLELIST);
    group->AllocateData(eVertexFormat::EVF_VERTEX, static_cast<int32>(vertices.size()), static_cast<int32>(indices.size()), segments * 2);
    memcpy(group->vertexArray, vertices.data(), vertices.size() * sizeof(Vector3));
    memcpy(group->indexArray, indices.data(), indices.size() * sizeof(uint16));
    group->BuildBuffers();
    group->RecalcAABBox();

    RenderObject* renderObject = new RenderObject();
    RenderBatch* batch = new RenderBatch();
    batch->SetMaterial(material.Get());
    batch->SetPolygonGroup(group.Get());
    batch->vertexLayoutId = static_cast<uint32>(-1);
    renderObject->AddRenderBatch(batch);

    return renderObject;
}

DAVA::Entity* CreateVehicleWheelEntity(DAVA::String name, DAVA::float32 radius, DAVA::float32 width, DAVA::float32 mass, DAVA::Vector3 localTranslation, DAVA::float32 maxHandbrakeTorque, DAVA::float32 maxSteerAngle)
{
    using namespace DAVA;

    Entity* wheel = new Entity();
    wheel->SetName(name.c_str());

    VehicleWheelComponent* wheelComponent = new VehicleWheelComponent();
    wheelComponent->SetRadius(radius);
    wheelComponent->SetWidth(width);
    wheelComponent->SetMaxHandbrakeTorque(maxHandbrakeTorque);
    wheelComponent->SetMaxSteerAngle(maxSteerAngle);
    wheel->AddComponent(wheelComponent);

    ConvexHullShapeComponent* shape = new ConvexHullShapeComponent();
    shape->SetOverrideMass(true);
    shape->SetMass(mass);
    wheel->AddComponent(shape);

    RenderComponent* wheel1Rendercomponent = new RenderComponent(CreateVehicleWheelRenderObject(radius, width));
    wheel->AddComponent(wheel1Rendercomponent);

    Matrix4 localTransform;
    localTransform.SetTranslationVector(localTranslation);
    wheel->SetLocalTransform(localTransform);

    return wheel;
}

class CarEntityCreator : public SimpleEntityCreator
{
    using TBase = SimpleEntityCreator;

public:
    static DAVA::RefPtr<DAVA::Entity> CreateEntity()
    {
        const Vector3 chassisHalfDimensions = Vector3(2.5f, 1.0f, 1.25f);
        const float32 chassisMass = 1500.0f;
        const float32 wheelRadius = 0.35f;
        const float32 wheelWidth = 0.4f;
        const float32 wheelMass = 20.0f;

        // Root entity
        Entity* vehicleEntity = new DAVA::Entity();
        vehicleEntity->SetName("Vehicle (car)");
        VehicleComponent* vehicleComponent = new VehicleCarComponent();
        vehicleEntity->AddComponent(vehicleComponent);
        DynamicBodyComponent* dynamicBody = new DynamicBodyComponent();
        vehicleEntity->AddComponent(dynamicBody);

        // Wheel (front left)
        Entity* vehicleWheel4Entity = CreateVehicleWheelEntity("Wheel (front left)", wheelRadius, wheelWidth, wheelMass, Vector3(chassisHalfDimensions.x * 0.7f, chassisHalfDimensions.y * 0.7f, -chassisHalfDimensions.z), 0.0f, PI * 0.333f);
        vehicleEntity->AddNode(vehicleWheel4Entity);

        // Wheel (front right)
        Entity* vehicleWheel3Entity = CreateVehicleWheelEntity("Wheel (front right)", wheelRadius, wheelWidth, wheelMass, Vector3(chassisHalfDimensions.x * 0.7f, -chassisHalfDimensions.y * 0.7f, -chassisHalfDimensions.z), 0.0f, PI * 0.333f);
        vehicleEntity->AddNode(vehicleWheel3Entity);

        // Wheel (rear left)
        Entity* vehicleWheel2Entity = CreateVehicleWheelEntity("Wheel (rear left)", wheelRadius, wheelWidth, wheelMass, Vector3(-chassisHalfDimensions.x * 0.7f, chassisHalfDimensions.y * 0.7f, -chassisHalfDimensions.z), 4000.0f, 0.0f);
        vehicleEntity->AddNode(vehicleWheel2Entity);

        // Wheel (rear right)
        Entity* vehicleWheel1Entity = CreateVehicleWheelEntity("Wheel (rear right)", wheelRadius, wheelWidth, wheelMass, Vector3(-chassisHalfDimensions.x * 0.7f, -chassisHalfDimensions.y * 0.7f, -chassisHalfDimensions.z), 4000.0f, 0.0f);
        vehicleEntity->AddNode(vehicleWheel1Entity);

        // Chassis
        ScopedPtr<Entity> vehicleChassisEntity(new Entity());
        vehicleChassisEntity->SetName("Chassis");
        VehicleChassisComponent* chassisComponent = new VehicleChassisComponent();
        vehicleChassisEntity->AddComponent(chassisComponent);
        BoxShapeComponent* chassisShape = new BoxShapeComponent();
        chassisShape->SetHalfSize(chassisHalfDimensions);
        chassisShape->SetOverrideMass(true);
        chassisShape->SetMass(chassisMass);
        vehicleChassisEntity->AddComponent(chassisShape);
        vehicleEntity->AddNode(vehicleChassisEntity);

        return DAVA::RefPtr<DAVA::Entity>(vehicleEntity);
    }

    CarEntityCreator()
        : TBase(eMenuPointOrder::PHYSICS_ENTITIES, QIcon(), QStringLiteral("Car"),
                DAVA::MakeFunction(&CarEntityCreator::CreateEntity))
    {
    }
};

class TankEntityCreator : public SimpleEntityCreator
{
    using TBase = SimpleEntityCreator;

public:
    static DAVA::RefPtr<DAVA::Entity> CreateEntity()
    {
        const Vector3 chassisHalfDimensions = Vector3(2.5f, 1.0f, 1.25f);
        const float32 chassisMass = 1500.0f;
        const float32 wheelRadius = 0.35f;
        const float32 wheelWidth = 0.4f;
        const float32 wheelMass = 20.0f;

        // Root entity
        Entity* vehicleEntity = new DAVA::Entity();
        vehicleEntity->SetName("Vehicle");
        VehicleComponent* vehicleComponent = new VehicleTankComponent();
        vehicleEntity->AddComponent(vehicleComponent);
        DynamicBodyComponent* dynamicBody = new DynamicBodyComponent();
        vehicleEntity->AddComponent(dynamicBody);

        // Wheels

        float mainwheelsz = -1.3f * chassisHalfDimensions.z;

        const uint32 wheelsNumber = 6;
        const float totalWheelsWidth = wheelWidth * wheelsNumber;

        float xOffset = -chassisHalfDimensions.x + (chassisHalfDimensions.x * 2.0f - wheelRadius * 2.0f);
        for (int i = 0; i < 6; ++i)
        {
            Entity* left = CreateVehicleWheelEntity("Wheel (left)", wheelRadius, wheelWidth, wheelMass, Vector3(xOffset, chassisHalfDimensions.y * 0.7f, mainwheelsz), 0.0f, 0.0f);
            vehicleEntity->AddNode(left);

            Entity* right = CreateVehicleWheelEntity("Wheel (right)", wheelRadius, wheelWidth, wheelMass, Vector3(xOffset, -chassisHalfDimensions.y * 0.7f, mainwheelsz), 0.0f, 0.0f);
            vehicleEntity->AddNode(right);

            xOffset -= wheelRadius * 2.0f;
        }

        // Chassis
        Entity* vehicleChassisEntity = new Entity();
        vehicleChassisEntity->SetName("Chassis");
        VehicleChassisComponent* chassisComponent = new VehicleChassisComponent();
        vehicleChassisEntity->AddComponent(chassisComponent);
        BoxShapeComponent* chassisShape = new BoxShapeComponent();
        chassisShape->SetHalfSize(chassisHalfDimensions);
        chassisShape->SetOverrideMass(true);
        chassisShape->SetMass(chassisMass);
        vehicleChassisEntity->AddComponent(chassisShape);
        vehicleEntity->AddNode(vehicleChassisEntity);

        return DAVA::RefPtr<DAVA::Entity>(vehicleEntity);
    }

    TankEntityCreator()
        : TBase(eMenuPointOrder::PHYSICS_ENTITIES, QIcon(), QStringLiteral("Tank"),
                DAVA::MakeFunction(&TankEntityCreator::CreateEntity))
    {
    }
};

class PhysicsEntityCreatorsGroup : public EntityCreatorsGroup
{
public:
    PhysicsEntityCreatorsGroup()
        : EntityCreatorsGroup(QIcon(), QStringLiteral("Physics"))
    {
        creatorsGroup.push_back(std::make_unique<CarEntityCreator>());
        creatorsGroup.push_back(std::make_unique<TankEntityCreator>());
    }

    eMenuPointOrder GetOrder() const override
    {
        return eMenuPointOrder::PHYSICS_ENTITIES;
    }

private:
    DAVA_VIRTUAL_REFLECTION_IN_PLACE(PhysicsEntityCreatorsGroup, EntityCreatorsGroup)
    {
        ReflectionRegistrator<PhysicsEntityCreatorsGroup>::Begin()
        .ConstructorByPointer()
        .End();
    }
};
} // namespace EditorPhysicsDetail

EditorPhysicsModule::EditorPhysicsModule()
{
    using namespace EditorPhysicsDetail;
    DAVA_REFLECTION_REGISTER_PERMANENT_NAME(PhysicsEntityCreatorsGroup);
}

void EditorPhysicsModule::OnContextCreated(DAVA::TArc::DataContext* context)
{
    SceneEditor2* scene = context->GetData<SceneData>()->GetScene().Get();
    DVASSERT(scene != nullptr);

    std::unique_ptr<EditorPhysicsData> data(new EditorPhysicsData());
    data->system = new EditorPhysicsSystem(scene);
    scene->AddSystem(data->system, 0, DAVA::Scene::SCENE_SYSTEM_REQUIRE_PROCESS);

    context->CreateData(std::move(data));
}

void EditorPhysicsModule::OnContextDeleted(DAVA::TArc::DataContext* context)
{
    SceneEditor2* scene = context->GetData<SceneData>()->GetScene().Get();
    DVASSERT(scene != nullptr);

    EditorPhysicsData* data = context->GetData<EditorPhysicsData>();
    DVASSERT(data != nullptr);
    DVASSERT(data->system != nullptr);
    scene->RemoveSystem(data->system);
    DAVA::SafeDelete(data->system);
    context->DeleteData<EditorPhysicsData>();
}

void EditorPhysicsModule::PostInit()
{
    using namespace DAVA;
    using namespace DAVA::TArc;

    ContextAccessor* accessor = GetAccessor();
    binder.reset(new FieldBinder(accessor));
    {
        FieldDescriptor descr;
        descr.type = ReflectedTypeDB::Get<ProjectManagerData>();
        descr.fieldName = FastName(ProjectManagerData::ProjectPathProperty);
        binder->BindField(descr, [](const Any& v) {
            PhysicsModule* module = GetEngineContext()->moduleManager->GetModule<PhysicsModule>();
            module->ReleaseMaterials();
        });
    }

    EditorPhysicsDetail::EditorPhysicsGlobalData* globalData = new EditorPhysicsDetail::EditorPhysicsGlobalData();
    globalData->physicsExtension.reset(new EditorPhysicsDetail::PhysicsEditorCreator());
    accessor->GetGlobalContext()->CreateData(std::unique_ptr<DAVA::TArc::DataNode>(globalData));

    QWidget* physicsPanel = new PhysicsWidget(GetAccessor(), GetUI());

    UI* ui = GetUI();

    DockPanelInfo info;
    info.title = QString("Physics");
    info.area = Qt::LeftDockWidgetArea;

    PanelKey key("Physics", info);
    ui->AddView(DAVA::TArc::mainWindowKey, key, physicsPanel);
}

void EditorPhysicsModule::OnInterfaceRegistered(const DAVA::Type* interfaceType)
{
    if (interfaceType == DAVA::Type::Instance<Interfaces::PropertyPanelInterface>())
    {
        Interfaces::PropertyPanelInterface* propertyPanel = QueryInterface<Interfaces::PropertyPanelInterface>();
        EditorPhysicsDetail::EditorPhysicsGlobalData* data = GetAccessor()->GetGlobalContext()->GetData<EditorPhysicsDetail::EditorPhysicsGlobalData>();
        propertyPanel->RegisterExtension(data->physicsExtension);
    }
}

void EditorPhysicsModule::OnBeforeInterfaceUnregistered(const DAVA::Type* interfaceType)
{
    if (interfaceType == DAVA::Type::Instance<Interfaces::PropertyPanelInterface>())
    {
        Interfaces::PropertyPanelInterface* propertyPanel = QueryInterface<Interfaces::PropertyPanelInterface>();
        EditorPhysicsDetail::EditorPhysicsGlobalData* data = GetAccessor()->GetGlobalContext()->GetData<EditorPhysicsDetail::EditorPhysicsGlobalData>();
        propertyPanel->UnregisterExtension(data->physicsExtension);
    }
}

DAVA_VIRTUAL_REFLECTION_IMPL(EditorPhysicsModule)
{
    DAVA::ReflectionRegistrator<EditorPhysicsModule>::Begin()
    .ConstructorByPointer()
    .End();
}

DECL_GUI_MODULE(EditorPhysicsModule);

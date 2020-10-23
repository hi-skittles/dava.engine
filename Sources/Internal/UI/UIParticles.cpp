#include "UI/UIParticles.h"
#include "Render/2D/Systems/RenderSystem2D.h"
#include "Scene3D/Components/ComponentHelpers.h"
#include "Scene3D/Components/ParticleEffectComponent.h"
#include "Scene3D/Systems/ParticleEffectSystem.h"
#include "Render/Highlevel/RenderPassNames.h"
#include "Reflection/ReflectionRegistrator.h"
#include "UI/UIControlSystem.h"
#include "UI/Update/UIUpdateComponent.h"

namespace DAVA
{
/* this camera is required just for preparing draw data*/
Camera* UIParticles::defaultCamera = nullptr;

DAVA_VIRTUAL_REFLECTION_IMPL(UIParticles)
{
    ReflectionRegistrator<UIParticles>::Begin()[M::DisplayName("Particles")]
    .ConstructorByPointer()
    .DestructorByPointer([](UIParticles* o) { o->Release(); })
    .Field("effectPath", &UIParticles::GetEffectPath, &UIParticles::SetEffectPath)[M::DisplayName("Effect Path")]
    .Field("autoStart", &UIParticles::IsAutostart, &UIParticles::SetAutostart)[M::DisplayName("Auto Start")]
    .Field("startDelay", &UIParticles::GetStartDelay, &UIParticles::SetStartDelay)[M::DisplayName("Start Delay")]
    .End();
}

UIParticles::UIParticles(const Rect& rect)
    : UIControl(rect)
{
    system = new ParticleEffectSystem(nullptr, true);
    if (defaultCamera != nullptr)
    {
        defaultCamera->Retain();
    }
    else
    {
        defaultCamera = new Camera();
        defaultCamera->SetPosition(-Vector3::UnitZ);
        defaultCamera->SetUp(-Vector3::UnitY);
        defaultCamera->RebuildCameraFromValues();
        defaultCamera->RebuildViewMatrix();
    }
    GetOrCreateComponent<UIUpdateComponent>();
}

UIParticles::~UIParticles()
{
    UnloadEffect();
    SafeDelete(system);

    if (defaultCamera->GetRetainCount() != 1)
    {
        defaultCamera->Release();
    }
    else
    {
        SafeRelease(defaultCamera);
    }
}

void UIParticles::OnActive()
{
    updateTime = 0.0f;
}

void UIParticles::Start()
{
    if (FLOAT_EQUAL(startDelay, 0.0f))
    {
        DoStart();
    }
    else
    {
        delayedActionType = actionStart;
        delayedActionTime = 0.0f;
    }
}

void UIParticles::DoStart()
{
    if (!effect)
    {
        return;
    }

    updateTime = 0.0f;

    if (effect->state == ParticleEffectComponent::STATE_STARTING ||
        effect->state == ParticleEffectComponent::STATE_PLAYING)
    {
        return;
    }

    effect->isPaused = false;
    system->AddToActive(effect);
    system->RunEffect(effect);
}

void UIParticles::Stop(bool isDeleteAllParticles)
{
    if (!effect)
    {
        return;
    }

    updateTime = 0.0f;

    if (effect->state == ParticleEffectComponent::STATE_STOPPED)
        return;

    if (isDeleteAllParticles)
    {
        effect->ClearCurrentGroups();
        effect->effectData.infoSources.resize(1);
        effect->isPaused = false;
        system->RemoveFromActive(effect);
    }
    else
    {
        effect->state = ParticleEffectComponent::STATE_STOPPING;
    }
}

void UIParticles::Pause(bool isPaused /*= true*/)
{
    if (!effect)
    {
        return;
    }

    effect->isPaused = isPaused;
}

bool UIParticles::IsStopped() const
{
    if (!effect)
    {
        return false;
    }

    return effect->state == ParticleEffectComponent::STATE_STOPPED;
}

bool UIParticles::IsPaused() const
{
    if (!effect)
    {
        return false;
    }

    return effect->isPaused;
}

void UIParticles::Restart(bool isDeleteAllParticles)
{
    delayedDeleteAllParticles = isDeleteAllParticles;
    if (FLOAT_EQUAL(startDelay, 0.0f))
    {
        DoRestart();
    }
    else
    {
        delayedActionType = actionRestart;
        delayedActionTime = 0.0f;
    }
}

void UIParticles::DoRestart()
{
    if (!effect)
    {
        return;
    }

    effect->isPaused = false;
    if (delayedDeleteAllParticles)
    {
        effect->ClearCurrentGroups();
    }

    effect->currRepeatsCont = 0;
    system->RunEffect(effect);
}

void UIParticles::Update(float32 timeElapsed)
{
    updateTime = timeElapsed;
    if (needHandleAutoStart)
    {
        needHandleAutoStart = false;
        HandleAutostart();
    }

    if (delayedActionType != UIParticles::actionNone)
    {
        HandleDelayedAction(timeElapsed);
    }
}

void UIParticles::Draw(const UIGeometricData& geometricData)
{
    if (!effect || effect->state == ParticleEffectComponent::STATE_STOPPED)
        return;

    RenderSystem2D::Instance()->Flush();

    system->Process(updateTime);
    updateTime = 0.0f;

    if (inheritControlTransform)
    {
        matrix = Matrix4::MakeScale(Vector3(geometricData.scale.x, geometricData.scale.y, 1.f)) * Matrix4::MakeRotation(Vector3::UnitZ, geometricData.angle);
        matrix.SetTranslationVector(Vector3(geometricData.position.x, geometricData.position.y, 0));
        Renderer::GetDynamicBindings().SetDynamicParam(DynamicBindings::PARAM_WORLD, &matrix, reinterpret_cast<pointer_size>(&matrix));

        effect->effectRenderObject->SetWorldMatrixPtr(&Matrix4::IDENTITY);
    }
    else
    {
        matrix.BuildRotation(Vector3::UnitZ, geometricData.angle);
        matrix.SetTranslationVector(Vector3(geometricData.position.x, geometricData.position.y, 0));
        effect->effectRenderObject->BindDynamicParameters(defaultCamera, nullptr);
        effect->effectRenderObject->SetWorldMatrixPtr(&matrix);
    }

    Renderer::GetDynamicBindings().SetDynamicParam(DynamicBindings::PARAM_CAMERA_POS, &Vector3::Zero, reinterpret_cast<pointer_size>(&Vector3::Zero));
    effect->effectRenderObject->PrepareToRender(defaultCamera);

    rhi::Packet packet;
    for (int32 i = 0, sz = effect->effectRenderObject->GetActiveRenderBatchCount(); i < sz; ++i)
    {
        RenderBatch* batch = effect->effectRenderObject->GetActiveRenderBatch(i);

        NMaterial* material = batch->GetMaterial();
        material->PreBuildMaterial(PASS_FORWARD);
        material->BindParams(packet);
        packet.vertexStreamCount = 1;
        packet.vertexStream[0] = batch->vertexBuffer;
        packet.baseVertex = batch->vertexBase;
        packet.vertexCount = batch->vertexCount;
        packet.indexBuffer = batch->indexBuffer;
        packet.primitiveType = batch->primitiveType;
        packet.primitiveCount = batch->indexCount / 3;
        packet.vertexLayoutUID = batch->vertexLayoutId;
        packet.startIndex = batch->startIndex;
        DVASSERT(packet.primitiveCount);
        packet.debugMarker = "UIParticles";
        RenderSystem2D::Instance()->DrawPacket(packet);
    }
}

void UIParticles::SetExtertnalValue(const String& name, float32 value)
{
    if (effect != nullptr)
        effect->SetExtertnalValue(name, value);
}

void UIParticles::SetInheritControlTransform(bool inherit)
{
    inheritControlTransform = inherit;

    if (!IsStopped())
    {
        Restart();
    }
}

bool UIParticles::GetInheritControlTransform() const
{
    return inheritControlTransform;
}

void UIParticles::LoadEffect(const FilePath& path)
{
    ScopedPtr<SceneFileV2> sceneFile(new SceneFileV2());
    sceneFile->EnableDebugLog(false);

    ScopedPtr<SceneArchive> archive(sceneFile->LoadSceneArchive(path));
    ParticleEffectComponent* newEffect = nullptr;
    if (static_cast<SceneArchive*>(archive) != nullptr && !archive->children.empty())
    {
        ScopedPtr<Entity> entity(new Entity());
        SerializationContext serializationContext;
        serializationContext.SetRootNodePath(path);
        serializationContext.SetScenePath(FilePath(path.GetDirectory()));
        serializationContext.SetVersion(10);
        serializationContext.SetScene(nullptr);
        serializationContext.SetDefaultMaterialQuality(NMaterialQualityName::DEFAULT_QUALITY_NAME);
        entity->Load(archive->children[0]->archive, &serializationContext);
        ParticleEffectComponent* effSrc = GetEffectComponent(entity);
        if (effSrc)
        {
            newEffect = static_cast<ParticleEffectComponent*>(effSrc->Clone(nullptr));
        }
    }

    if (newEffect)
    {
        DVASSERT(!effect);
        effect = newEffect;
        needHandleAutoStart = true;
    }
}

void UIParticles::UnloadEffect()
{
    if (!effect)
        return;

    if (effect->state != ParticleEffectComponent::STATE_STOPPED)
        system->RemoveFromActive(effect);

    SafeDelete(effect);
}

void UIParticles::ReloadEffect()
{
    if (effectPath.IsEmpty())
    {
        DVASSERT(false, "You have to load UIPartilces effect prior to calling Reload()");
        return;
    }

    UnloadEffect();
    LoadEffect(effectPath);
}

void UIParticles::SetEffectPath(const FilePath& path)
{
    effectPath = path;
    UnloadEffect();
    if (!effectPath.IsEmpty())
    {
        LoadEffect(effectPath);
    }
}

const FilePath& UIParticles::GetEffectPath() const
{
    return effectPath;
}

void UIParticles::SetAutostart(bool value)
{
    isAutostart = value;
    needHandleAutoStart = true;
}

bool UIParticles::IsAutostart() const
{
    return isAutostart;
}

UIParticles* UIParticles::Clone()
{
    UIParticles* particles = new UIParticles(GetRect());
    particles->CopyDataFrom(this);
    return particles;
}

void UIParticles::CopyDataFrom(UIControl* srcControl)
{
    UIControl::CopyDataFrom(srcControl);
    UIParticles* src = static_cast<UIParticles*>(srcControl);

    SetEffectPath(src->GetEffectPath());
    SetStartDelay(src->GetStartDelay());
    SetAutostart(src->IsAutostart());
    SetInheritControlTransform(src->GetInheritControlTransform());
}

void UIParticles::HandleAutostart()
{
    if (isAutostart && effect)
    {
        Start();
    }
}

float32 UIParticles::GetStartDelay() const
{
    return startDelay;
}

void UIParticles::SetStartDelay(float32 value)
{
    startDelay = value;
}

void UIParticles::HandleDelayedAction(float32 timeElapsed)
{
    if (IsVisible())
    {
        delayedActionTime += timeElapsed;
        if (delayedActionTime >= startDelay)
        {
            switch (delayedActionType)
            {
            case UIParticles::actionStart:
            {
                DoStart();
                break;
            }

            case UIParticles::actionRestart:
            {
                DoRestart();
                break;
            }

            default:
            {
                break;
            }
            }

            delayedActionType = UIParticles::actionNone;
            delayedActionTime = 0.0f;
        }
    }
}
};

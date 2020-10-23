#include "Classes/Qt/Scene/System/LandscapeEditorDrawSystem/CustomColorsProxy.h"
#include "Classes/Deprecated/EditorConfig.h"

#include "Classes/Project/ProjectManagerData.h"
#include "Classes/Application/REGlobal.h"
#include "Classes/SceneManager/SceneData.h"

#include <Render/Texture.h>
#include <Render/Material/NMaterial.h>

CustomColorsProxy::CustomColorsProxy(DAVA::int32 _size)
    : changedRect(DAVA::Rect())
    , spriteChanged(false)
    , textureLoaded(false)
    , size(_size)
    , changes(0)
    , brushMaterial(new DAVA::NMaterial())
{
    DAVA::Texture::FBODescriptor fboDesc;
    fboDesc.width = size;
    fboDesc.height = size;
    fboDesc.textureType = rhi::TextureType::TEXTURE_TYPE_2D;
    fboDesc.format = DAVA::PixelFormat::FORMAT_RGBA8888;
    fboDesc.needDepth = false;
    fboDesc.needPixelReadback = true;
    customColorsRenderTarget = DAVA::Texture::CreateFBO(fboDesc);

    // clear texture, to initialize frame buffer object
    // using PRIORITY_SERVICE_2D + 1 to ensure it will be cleared before drawing existing image into render target
    DAVA::RenderHelper::CreateClearPass(customColorsRenderTarget->handle, rhi::HTexture(), DAVA::PRIORITY_SERVICE_2D + 1, DAVA::Color::Clear, rhi::Viewport(0, 0, size, size));

    brushMaterial->SetMaterialName(DAVA::FastName("CustomColorsMaterial"));
    brushMaterial->SetFXName(DAVA::FastName("~res:/ResourceEditor/LandscapeEditor/Materials/CustomColors.material"));
    brushMaterial->PreBuildMaterial(DAVA::RenderSystem2D::RENDER_PASS_NAME);
}

void CustomColorsProxy::ResetLoadedState(bool isLoaded)
{
    textureLoaded = isLoaded;
}

bool CustomColorsProxy::IsTextureLoaded() const
{
    return textureLoaded;
}

CustomColorsProxy::~CustomColorsProxy()
{
    SafeRelease(customColorsRenderTarget);
}

DAVA::Texture* CustomColorsProxy::GetTexture()
{
    return customColorsRenderTarget;
}

void CustomColorsProxy::ResetTargetChanged()
{
    spriteChanged = false;
}

bool CustomColorsProxy::IsTargetChanged()
{
    return spriteChanged;
}

DAVA::Rect CustomColorsProxy::GetChangedRect()
{
    if (IsTargetChanged())
    {
        return changedRect;
    }

    return DAVA::Rect();
}

void CustomColorsProxy::UpdateRect(const DAVA::Rect& rect)
{
    DAVA::Rect bounds(0.f, 0.f, static_cast<DAVA::float32>(size), static_cast<DAVA::float32>(size));
    changedRect = rect;
    bounds.ClampToRect(changedRect);

    spriteChanged = true;
}

DAVA::int32 CustomColorsProxy::GetChangesCount() const
{
    return changes;
}

void CustomColorsProxy::ResetChanges()
{
    changes = 0;
}

void CustomColorsProxy::IncrementChanges()
{
    ++changes;
}

void CustomColorsProxy::DecrementChanges()
{
    --changes;
}

void CustomColorsProxy::UpdateSpriteFromConfig()
{
    if (NULL == customColorsRenderTarget)
    {
        return;
    }

    rhi::Viewport viewport;
    viewport.x = viewport.y = 0;
    viewport.width = viewport.height = size;

    ProjectManagerData* data = REGlobal::GetDataNode<ProjectManagerData>();
    DVASSERT(data);

    DAVA::Vector<DAVA::Color> customColors = data->GetEditorConfig()->GetColorPropertyValues("LandscapeCustomColors");
    if (customColors.empty())
    {
        DAVA::RenderHelper::CreateClearPass(customColorsRenderTarget->handle, rhi::HTexture(), DAVA::PRIORITY_CLEAR, DAVA::Color::Clear, viewport);
    }
    else
    {
        DAVA::uint32 defaultColorIndex = REGlobal::GetGlobalContext()->GetData<GlobalSceneSettings>()->defaultCustomColorIndex;
        defaultColorIndex = DAVA::Min(defaultColorIndex, static_cast<DAVA::uint32>(customColors.size() - 1));
        DAVA::Color color = customColors[defaultColorIndex];
        DAVA::RenderHelper::CreateClearPass(customColorsRenderTarget->handle, rhi::HTexture(), DAVA::PRIORITY_CLEAR, color, viewport);
    }
}

DAVA::NMaterial* CustomColorsProxy::GetBrushMaterial() const
{
    return brushMaterial.get();
}
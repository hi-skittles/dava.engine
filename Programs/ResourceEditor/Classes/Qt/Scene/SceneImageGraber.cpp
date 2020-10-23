#include "SceneImageGraber.h"

#include "Engine/Engine.h"
#include "Functional/Function.h"
#include "Job/JobManager.h"
#include "Math/MathHelpers.h"
#include "Render/RHI/rhi_Type.h"
#include "Render/RHI/rhi_Public.h"
#include "Render/RenderBase.h"
#include "Render/Renderer.h"
#include "Render/2D/Systems/RenderSystem2D.h"
#include "Render/Highlevel/RenderSystem.h"
#include "Render/Image/Image.h"
#include "Render/Image/ImageSystem.h"

#include "Scene3D/Scene.h"

#include <QQuickWindow>
#include <QImage>

namespace SceneImageGrabber
{
namespace SceneImageGrabberDetail
{
struct InternalParams
{
    SceneImageGrabber::Params inputParams;
    DAVA::RefPtr<DAVA::Texture> renderTarget;
};

void GrabImage(Params inputParams)
{
    DVASSERT(!inputParams.outputFile.IsEmpty());
    InternalParams internalParams;
    internalParams.inputParams = std::move(inputParams);

    DAVA::Size2i imageSize = internalParams.inputParams.imageSize;
    DAVA::int32 maxSize = std::max(imageSize.dx, imageSize.dy);
    internalParams.renderTarget = DAVA::Texture::CreateFBO(maxSize, maxSize, DAVA::PixelFormat::FORMAT_RGBA8888, true);

    DAVA::float32 cameraAspect = internalParams.inputParams.cameraToGrab->GetAspect();
    internalParams.inputParams.cameraToGrab->SetAspect(static_cast<DAVA::float32>(imageSize.dx) / imageSize.dy);
    internalParams.inputParams.scene->renderSystem->SetDrawCamera(internalParams.inputParams.cameraToGrab.Get());
    internalParams.inputParams.scene->renderSystem->SetMainCamera(internalParams.inputParams.cameraToGrab.Get());

    DAVA::Rect viewportRect(0, 0, imageSize.dx, imageSize.dy);
    internalParams.inputParams.scene->SetMainRenderTarget(internalParams.renderTarget->handle,
                                                          internalParams.renderTarget->handleDepthStencil,
                                                          rhi::LOADACTION_CLEAR, DAVA::Color::Clear);
    internalParams.inputParams.scene->SetMainPassProperties(DAVA::PRIORITY_SERVICE_2D, viewportRect,
                                                            internalParams.renderTarget->GetWidth(),
                                                            internalParams.renderTarget->GetHeight(),
                                                            DAVA::PixelFormat::FORMAT_RGBA8888);
    internalParams.inputParams.scene->Draw();

    DAVA::RenderSystem2D* renderSystem = DAVA::RenderSystem2D::Instance();

    renderSystem->BeginRenderTargetPass(internalParams.renderTarget.Get(), false);
    renderSystem->FillRect(viewportRect, DAVA::Color::White, DAVA::RenderSystem2D::DEFAULT_2D_FILL_ALPHA_MATERIAL);
    renderSystem->EndRenderTargetPass();

    internalParams.inputParams.cameraToGrab->SetAspect(cameraAspect);

    DAVA::Renderer::RegisterSyncCallback(rhi::GetCurrentFrameSyncObject(), [internalParams](rhi::HSyncObject)
                                         {
                                             DAVA::FilePath filePath = internalParams.inputParams.outputFile;
                                             if (filePath.IsDirectoryPathname())
                                             {
                                                 filePath = DAVA::FilePath(filePath, "GrabbedScene.png");
                                             }

                                             DAVA::ScopedPtr<DAVA::Image> image(internalParams.renderTarget->CreateImageFromMemory());
                                             DAVA::Size2i imageSize = internalParams.inputParams.imageSize;
                                             DAVA::ScopedPtr<DAVA::Image> regionImage(DAVA::Image::CopyImageRegion(image, DAVA::Rect(0, 0, imageSize.dx, imageSize.dy)));
                                             regionImage->Save(filePath);
                                             if (internalParams.inputParams.readyCallback)
                                             {
                                                 internalParams.inputParams.readyCallback();
                                             }
                                         });
}
}

void GrabImage(Params params)
{
    if (params.processInDAVAFrame)
    {
        DAVA::GetEngineContext()->jobManager->CreateMainJob(DAVA::Bind(&SceneImageGrabberDetail::GrabImage, params), DAVA::JobManager::eMainJobType::JOB_MAINLAZY);
    }
    else
    {
        SceneImageGrabberDetail::GrabImage(params);
    }
}
}

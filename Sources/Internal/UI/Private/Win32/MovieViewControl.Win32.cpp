#include "Base/Platform.h"

#if defined(__DAVAENGINE_WIN32__)
#if !defined(DISABLE_NATIVE_MOVIEVIEW)

#include "UI/Private/Win32/FfmpegPlayer.Win32.h"
#include "UI/Private/Win32/MovieViewControl.Win32.h"
#include "Render/2D/Sprite.h"
#include "Render/Image/Image.h"
#include "Render/PixelFormatDescriptor.h"
#include "Render/Texture.h"
#include "UI/UIControl.h"
#include "UI/UIControlBackground.h"

namespace DAVA
{
MovieViewControl::MovieViewControl(Window* /*w*/)
    : ffmpegPlayer(new FfmpegPlayer())
    , videoBackground(new UIControlBackground())
{
}

MovieViewControl::~MovieViewControl()
{
    SafeRelease(videoTexture);
}

void MovieViewControl::Initialize(const Rect& rect)
{
    controlRect = rect;
    ffmpegPlayer->Initialize(rect);
}

void MovieViewControl::SetRect(const Rect& rect)
{
    controlRect = rect;
}

void MovieViewControl::SetVisible(bool isVisible)
{
}

void MovieViewControl::OpenMovie(const FilePath& moviePath, const OpenMovieParams& params)
{
    SafeRelease(videoTexture);

    ffmpegPlayer->OpenMovie(moviePath, params);

    scaling = params.scalingMode;
    switch (params.scalingMode)
    {
    case scalingModeNone:
        videoBackground->SetDrawType(UIControlBackground::eDrawType::DRAW_ALIGNED);
        videoBackground->SetAlign(ALIGN_LEFT | ALIGN_TOP);
        break;
    case scalingModeAspectFit:
        videoBackground->SetDrawType(UIControlBackground::eDrawType::DRAW_SCALE_PROPORTIONAL);
        videoBackground->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
        break;
    case scalingModeAspectFill:
        videoBackground->SetDrawType(UIControlBackground::eDrawType::DRAW_SCALE_PROPORTIONAL_ONE);
        videoBackground->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
        break;
    case scalingModeFill:
        videoBackground->SetDrawType(UIControlBackground::eDrawType::DRAW_SCALE_TO_RECT);
        videoBackground->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
        break;
    default:
        break;
    }
}

void MovieViewControl::Play()
{
    ffmpegPlayer->Play();
    Vector2 res = ffmpegPlayer->GetResolution();
    textureWidth = NextPowerOf2(static_cast<uint32>(res.dx));
    textureHeight = NextPowerOf2(static_cast<uint32>(res.dy));
    uint32 size = ImageUtils::GetSizeInBytes(textureWidth, textureHeight, ffmpegPlayer->GetPixelFormat());

    videoTextureBuffer.resize(size);
    Memset(videoTextureBuffer.data(), 0, size);
}

void MovieViewControl::Stop()
{
    ffmpegPlayer->Stop();
    SafeRelease(videoTexture);
}

void MovieViewControl::Pause()
{
    ffmpegPlayer->Pause();
}

void MovieViewControl::Resume()
{
    ffmpegPlayer->Resume();
}

eMoviePlayingState MovieViewControl::GetState() const
{
    switch (ffmpegPlayer->GetState())
    {
    case FfmpegPlayer::PLAYING:
        return eMoviePlayingState::statePlaying;
    case FfmpegPlayer::PREFETCHING:
        return eMoviePlayingState::stateLoading;
    case FfmpegPlayer::PAUSED:
        return eMoviePlayingState::statePaused;
    case FfmpegPlayer::STOPPED:
    default:
        return eMoviePlayingState::stateStopped;
    }
}

void MovieViewControl::Update()
{
    if (nullptr == ffmpegPlayer)
    {
        return;
    }

    ffmpegPlayer->Update();

    if (FfmpegPlayer::STOPPED == ffmpegPlayer->GetState())
    {
        SafeRelease(videoTexture);
    }

    FfmpegPlayer::DrawVideoFrameData drawData = ffmpegPlayer->GetDrawData();

    if (PixelFormatDescriptor::TEXTURE_FORMAT_INVALID == drawData.format || 0 == drawData.data.size())
    {
        return;
    }

    // Copy image data from video frame to texture buffer
    const uint32 pixelSize = PixelFormatDescriptor::GetPixelFormatSizeInBits(drawData.format) / 8;
    const uint32 textureLine = textureWidth * pixelSize;
    const uint32 frameLine = drawData.frameWidth * pixelSize;
    DVASSERT(textureWidth >= drawData.frameWidth && textureHeight >= drawData.frameHeight, "Incorrect frame size!");
    for (uint32 y = 0; y < drawData.frameHeight; ++y)
    {
        uint8* dst = videoTextureBuffer.data() + y * textureLine;
        uint8* src = drawData.data.data() + y * frameLine;
        Memcpy(dst, src, frameLine);
    }

    if (nullptr == videoTexture)
    {
        videoTexture = Texture::CreateFromData(drawData.format, reinterpret_cast<uint8*>(videoTextureBuffer.data()), textureWidth, textureHeight, false);

        Rect textureRectToMap;
        Vector2 spriteSizeFill;

        switch (scaling)
        {
        case scalingModeAspectFill:
        {
            float32 drawingRectAspect = static_cast<float32>(controlRect.dx) / controlRect.dy;
            float32 frameAspect = static_cast<float32>(drawData.frameWidth) / drawData.frameHeight;

            if (drawingRectAspect > frameAspect)
            {
                float32 visibleFrameHeight = drawData.frameWidth / drawingRectAspect;
                float32 dh = drawData.frameHeight - visibleFrameHeight;
                float32 dy = dh / 2;
                textureRectToMap = Rect(0, dy, static_cast<float32>(drawData.frameWidth), visibleFrameHeight);
            }
            else
            {
                float32 visibleFrameWidth = drawData.frameHeight * drawingRectAspect;
                float32 dl = drawData.frameWidth - visibleFrameWidth;
                float32 dx = dl / 2;
                textureRectToMap = Rect(dx, 0, visibleFrameWidth, static_cast<float32>(drawData.frameHeight));
            }

            spriteSizeFill = Vector2(static_cast<float32>(controlRect.dx), static_cast<float32>(controlRect.dy));
        }
        break;
        case scalingModeNone:
            textureRectToMap = Rect(0, 0, controlRect.dx, controlRect.dy);
            spriteSizeFill = Vector2(controlRect.dx, controlRect.dy);
            break;
        case scalingModeFill:
        case scalingModeAspectFit:
            textureRectToMap = Rect(0, 0, static_cast<float32>(drawData.frameWidth), static_cast<float32>(drawData.frameHeight));
            spriteSizeFill = Vector2(static_cast<float32>(drawData.frameWidth), static_cast<float32>(drawData.frameHeight));
            break;
        }

        uint32 tx = static_cast<uint32>(textureRectToMap.x);
        uint32 ty = static_cast<uint32>(textureRectToMap.y);
        uint32 tdx = static_cast<uint32>(textureRectToMap.dx);
        uint32 tdy = static_cast<uint32>(textureRectToMap.dy);

        Sprite* videoSprite = Sprite::CreateFromTexture(videoTexture, tx, ty, tdx, tdy, spriteSizeFill.dx, spriteSizeFill.dy);
        videoBackground->SetSprite(videoSprite);
        videoSprite->Release();
    }
    else
    {
        videoTexture->TexImage(0, textureWidth, textureHeight, reinterpret_cast<uint8*>(videoTextureBuffer.data()), static_cast<uint32>(drawData.data.size()), Texture::INVALID_CUBEMAP_FACE);
    }
}

void MovieViewControl::Draw(const UIGeometricData& parentGeometricData)
{
    if (nullptr != videoTexture)
    {
        Polygon2 poly;

        parentGeometricData.GetPolygon(poly);

        auto sz = videoBackground->GetSprite()->GetSize();

        //videoBackground->GetSprite()->SetClipPolygon(&poly);

        videoBackground->Draw(parentGeometricData);

        //videoBackground->GetSprite()->SetClipPolygon(nullptr);
    }
}
}

#endif // !DISABLE_NATIVE_MOVIEVIEW
#endif // __DAVAENGINE_WIN32__

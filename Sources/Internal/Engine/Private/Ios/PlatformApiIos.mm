#include "Base/Platform.h"

#if defined(__DAVAENGINE_IPHONE__)

#include "Engine/Private/EngineBackend.h"
#include "Engine/Private/Ios/PlatformCoreIos.h"
#include "Engine/Private/Ios/CoreNativeBridgeIos.h"
#include "Engine/Private/Ios/WindowImplIos.h"
#include "Engine/Private/Ios/WindowNativeBridgeIos.h"
#include "Render/Image/Image.h"

#import <UIKit/UIKit.h>

namespace DAVA
{
namespace PlatformApi
{
namespace Ios
{
void AddUIView(Window* targetWindow, UIView* uiview)
{
    using namespace DAVA::Private;
    WindowImpl* wb = EngineBackend::GetWindowImpl(targetWindow);
    wb->bridge->AddUIView(uiview);
}

void RemoveUIView(Window* targetWindow, UIView* uiview)
{
    using namespace DAVA::Private;
    WindowImpl* wb = EngineBackend::GetWindowImpl(targetWindow);
    wb->bridge->RemoveUIView(uiview);
}

UIView* GetUIViewFromPool(Window* targetWindow, const char8* className)
{
    using namespace DAVA::Private;
    WindowImpl* wb = EngineBackend::GetWindowImpl(targetWindow);
    return wb->bridge->GetUIViewFromPool(className);
}

void ReturnUIViewToPool(Window* targetWindow, UIView* view)
{
    using namespace DAVA::Private;
    WindowImpl* wb = EngineBackend::GetWindowImpl(targetWindow);
    wb->bridge->ReturnUIViewToPool(view);
}

UIImage* RenderUIViewToUIImage(UIView* view)
{
    DVASSERT(view != nullptr);

    UIImage* image = nullptr;
    CGSize size = view.frame.size;
    if (size.width > 0 && size.height > 0)
    {
        UIGraphicsBeginImageContextWithOptions(size, NO, 0);
        // Workaround! iOS bug see http://stackoverflow.com/questions/23157653/drawviewhierarchyinrectafterscreenupdates-delays-other-animations
        [view.layer renderInContext:UIGraphicsGetCurrentContext()];

        image = UIGraphicsGetImageFromCurrentImageContext();
        UIGraphicsEndImageContext();
    }
    return image;
}

Image* ConvertUIImageToImage(UIImage* nativeImage)
{
    DVASSERT(nativeImage != nullptr);

    Image* image = nullptr;
    CGImageRef imageRef = [nativeImage CGImage];
    if (imageRef != nullptr)
    {
        uint32 width = static_cast<uint32>(CGImageGetWidth(imageRef));
        uint32 height = static_cast<uint32>(CGImageGetHeight(imageRef));

        image = Image::Create(width, height, DAVA::FORMAT_RGBA8888);
        if (image != nullptr)
        {
            uint8* rawData = image->GetData();

            const uint32 bytesPerPixel = 4;
            const uint32 bitsPerComponent = 8;
            const uint32 bytesPerRow = bytesPerPixel * width;

            Memset(rawData, 0, width * height * bytesPerPixel);

            CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
            CGContextRef context = CGBitmapContextCreate(rawData,
                                                         width,
                                                         height,
                                                         bitsPerComponent,
                                                         bytesPerRow,
                                                         colorSpace,
                                                         kCGImageAlphaPremultipliedLast | kCGBitmapByteOrder32Big);

            CGContextDrawImage(context, CGRectMake(0, 0, width, height), imageRef);
            CGContextRelease(context);
            CGColorSpaceRelease(colorSpace);
        }
        //CGImageRelease(imageRef);
    }
    return image;
}

void RegisterDVEApplicationListener(id<DVEApplicationListener> listener)
{
    using namespace DAVA::Private;
    PlatformCore* core = EngineBackend::Instance()->GetPlatformCore();
    core->bridge->RegisterDVEApplicationListener(listener);
}

void UnregisterDVEApplicationListener(id<DVEApplicationListener> listener)
{
    using namespace DAVA::Private;
    PlatformCore* core = EngineBackend::Instance()->GetPlatformCore();
    core->bridge->UnregisterDVEApplicationListener(listener);
}

} // namespace Ios
} // namespace PlatformApi
} // namespace DAVA

#endif // defined(__DAVAENGINE_IPHONE__)

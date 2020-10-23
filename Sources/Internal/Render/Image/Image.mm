#import "Image.h"

#ifdef __DAVAENGINE_IPHONE__

#import <Foundation/Foundation.h>
#import <AssetsLibrary/AssetsLibrary.h>
#import <UIKit/UIKit.h>

#import "Render/Texture.h"
#import "Render/PixelFormatDescriptor.h"

namespace DAVA
{
void Image::SaveToSystemPhotos(SaveToSystemPhotoCallbackReceiver* callback)
{
    DVASSERT(format == FORMAT_RGBA8888);

    static const size_t bitsPerComponent = 8;
    static const size_t bitsPerPixel = 32;
    static const size_t bytesPerPixel = 4;
    size_t bytesPerRow = width * bytesPerPixel;
    CGColorSpaceRef colorSpaceRef = CGColorSpaceCreateDeviceRGB();
    CGBitmapInfo bitmapInfo = kCGBitmapByteOrderDefault;
    CGColorRenderingIntent renderingIntent = kCGRenderingIntentDefault;

    CGDataProviderRef provider = CGDataProviderCreateWithData(NULL, data, width * height * bytesPerPixel, NULL);

    CGImageRef imageRef = CGImageCreate(width, height, bitsPerComponent, bitsPerPixel, bytesPerRow, colorSpaceRef, bitmapInfo, provider, NULL, NO, renderingIntent);

    UIImage* image = [UIImage imageWithCGImage:imageRef];

    ALAssetsLibrary* library = [[ALAssetsLibrary alloc] init];

    if (callback != 0)
    {
        [library writeImageToSavedPhotosAlbum:[image CGImage]
                                  orientation:(ALAssetOrientation)[image imageOrientation]
                              completionBlock:^(NSURL* assetURL, NSError* error)
                                              {
                                                callback->SaveToSystemPhotosFinished();
                                              }];
    }
    else
    {
        [library writeImageToSavedPhotosAlbum:[image CGImage] orientation:(ALAssetOrientation)[image imageOrientation] completionBlock:NULL];
    }

    [library release];
}

void* Image::GetUIImage()
{
    size_t bitsPerComponent = 8;
    size_t bitsPerPixel = PixelFormatDescriptor::GetPixelFormatSizeInBits(format);
    size_t bytesPerPixel = bitsPerPixel / 8;
    size_t bytesPerRow = width * bytesPerPixel;
    CGColorSpaceRef colorSpaceRef = CGColorSpaceCreateDeviceRGB();
    CGBitmapInfo bitmapInfo = kCGBitmapByteOrderDefault;
    CGColorRenderingIntent renderingIntent = kCGRenderingIntentDefault;

    CGDataProviderRef provider = CGDataProviderCreateWithData(NULL, data, width * height * bytesPerPixel, NULL);
    CGImageRef imageRef = CGImageCreate(width, height, bitsPerComponent, bitsPerPixel, bytesPerRow, colorSpaceRef, bitmapInfo, provider, NULL, NO, renderingIntent);

    UIImage* image = [UIImage imageWithCGImage:imageRef];
    CGImageRelease(imageRef);
    CGDataProviderRelease(provider);

    return image;
}
}

#endif //__DAVAENGINE_IPHONE_

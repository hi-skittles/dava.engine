#if !defined __METAL_H__
#define __METAL_H__

#define RHI_METAL__USE_NATIVE_COMMAND_BUFFERS 0

#if !(TARGET_IPHONE_SIMULATOR == 1)
#include <Metal/Metal.h>
#include <QuartzCore/CAMetalLayer.h>
#include "Concurrency/Semaphore.h"

namespace rhi
{
extern id<MTLDevice> _Metal_Device;
extern id<MTLCommandQueue> _Metal_DefCmdQueue;
extern id<MTLTexture> _Metal_DefFrameBuf;
extern id<MTLTexture> _Metal_DefDepthBuf;
extern id<MTLTexture> _Metal_DefStencilBuf;
extern id<MTLDepthStencilState> _Metal_DefDepthState;
extern CAMetalLayer* _Metal_Layer;
extern DAVA::Semaphore* _Metal_DrawableDispatchSemaphore;
}

#endif

#endif // __METAL_H__

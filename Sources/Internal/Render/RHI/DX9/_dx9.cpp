#include "_dx9.h"
#include "Concurrency/Mutex.h"

#pragma warning(disable : 7 193 271 304 791)
#include <d3d9.h>
#pragma warning(default : 7 193 271 304 791)
#include <stdio.h>

#include "../rhi_Public.h"

//==============================================================================

namespace rhi
{
IDirect3D9* _D3D9 = nullptr;
IDirect3DDevice9* _D3D9_Device = nullptr;
unsigned _D3D9_TargetCount = 1;
IDirect3DSurface9* _D3D9_BackBuf = nullptr;
IDirect3DSurface9* _D3D9_DepthBuf = nullptr;

InitParam _DX9_InitParam;
D3DPRESENT_PARAMETERS _DX9_PresentParam;
RECT _DX9_PresentRect;
PRECT _DX9_PresentRectPtr = nullptr;
DAVA::Mutex _DX9_ResetParamsMutex;

const char* D3D9ErrorText(HRESULT hr)
{
#define D3DCASE(ERR) case ERR: return #ERR ": "
    switch (hr)
    {
        D3DCASE(D3D_OK)
        "No error occurred";
        D3DCASE(D3DOK_NOAUTOGEN)
        "This is a success code. However, the autogeneration of mipmaps is not supported for this format. This means that resource creation will succeed but the mipmap levels will not be automatically generated";
        D3DCASE(D3DERR_CONFLICTINGRENDERSTATE)
        "The currently set render states cannot be used together";
        D3DCASE(D3DERR_CONFLICTINGTEXTUREFILTER)
        "The current texture filters cannot be used together";
        D3DCASE(D3DERR_CONFLICTINGTEXTUREPALETTE)
        "The current textures cannot be used simultaneously";
        D3DCASE(D3DERR_DEVICELOST)
        "The device has been lost but cannot be reset at this time. Therefore, rendering is not possible";
        D3DCASE(D3DERR_DEVICENOTRESET)
        "The device has been lost but can be reset at this time";
        D3DCASE(D3DERR_DRIVERINTERNALERROR)
        "Internal driver error. Applications should destroy and recreate the device when receiving this error. For hints on debugging this error, see Driver Internal Errors";
        D3DCASE(D3DERR_DRIVERINVALIDCALL)
        "Not used";
        D3DCASE(D3DERR_INVALIDCALL)
        "The method call is invalid. For example, a method's parameter may not be a valid pointer";
        D3DCASE(D3DERR_INVALIDDEVICE)
        "The requested device type is not valid";
        D3DCASE(D3DERR_MOREDATA)
        "There is more data available than the specified buffer size can hold";
        D3DCASE(D3DERR_NOTAVAILABLE)
        "This device does not support the queried technique";
        D3DCASE(D3DERR_NOTFOUND)
        "The requested item was not found";
        D3DCASE(D3DERR_OUTOFVIDEOMEMORY)
        "Direct3D does not have enough display memory to perform the operation";
        D3DCASE(D3DERR_TOOMANYOPERATIONS)
        "The application is requesting more texture-filtering operations than the device supports";
        D3DCASE(D3DERR_UNSUPPORTEDALPHAARG)
        "The device does not support a specified texture-blending argument for the alpha channel";
        D3DCASE(D3DERR_UNSUPPORTEDALPHAOPERATION)
        "The device does not support a specified texture-blending operation for the alpha channel";
        D3DCASE(D3DERR_UNSUPPORTEDCOLORARG)
        "The device does not support a specified texture-blending argument for color values";
        D3DCASE(D3DERR_UNSUPPORTEDCOLOROPERATION)
        "The device does not support a specified texture-blending operation for color values";
        D3DCASE(D3DERR_UNSUPPORTEDFACTORVALUE)
        "The device does not support the specified texture factor value. Not used; provided only to support older drivers";
        D3DCASE(D3DERR_UNSUPPORTEDTEXTUREFILTER)
        "The device does not support the specified texture filter";
        D3DCASE(D3DERR_WASSTILLDRAWING)
        "The previous blit operation that is transferring information to or from this surface is incomplete";
        D3DCASE(D3DERR_WRONGTEXTUREFORMAT)
        "The pixel format of the texture surface is not valid";
        D3DCASE(E_FAIL)
        "An undetermined error occurred inside the Direct3D subsystem";
        D3DCASE(E_INVALIDARG)
        "An invalid parameter was passed to the returning function";
        D3DCASE(E_NOINTERFACE)
        "No object interface is available";
        D3DCASE(E_NOTIMPL)
        "Not implemented";
        D3DCASE(E_OUTOFMEMORY)
        "Direct3D could not allocate sufficient memory to complete the call";
        D3DCASE(D3DERR_DEVICEHUNG)
        "The device that returned this code caused the hardware adapter to be reset by the OS.Most applications should destroy the device and quit.Applications that must continue should destroy all video memory objects(surfaces, textures, state blocks etc) and call Reset() to put the device in a default state.If the application then continues rendering in the same way, the device will return to this state.";
        D3DCASE(D3DERR_DEVICEREMOVED)
        "The hardware adapter has been removed.Application must destroy the device, do enumeration of adapters and create another Direct3D device.If application continues rendering without calling Reset, the rendering calls will succeed.";
        D3DCASE(S_NOT_RESIDENT)
        "At least one allocation that comprises the resources is on disk.Direct3D 9Ex only.";
        D3DCASE(S_RESIDENT_IN_SHARED_MEMORY)
        "No allocations that comprise the resources are on disk.However, at least one allocation is not in GPU - accessible memory.Direct3D 9Ex only.";
        D3DCASE(S_PRESENT_MODE_CHANGED)
        "The desktop display mode has been changed.The application can continue rendering, but there might be color conversion / stretching.Pick a back buffer format similar to the current display mode, and call Reset to recreate the swap chains.The device will leave this state after a Reset is called.Direct3D 9Ex only.";
        D3DCASE(S_PRESENT_OCCLUDED)
        "The presentation area is occluded.Occlusion means that the presentation window is minimized or another device entered the fullscreen mode on the same monitor as the presentation window and the presentation window is completely on that monitor.Occlusion will not occur if the client area is covered by another Window. Occluded applications can continue rendering and all calls will succeed, but the occluded presentation window will not be updated.Preferably the application should stop rendering to the presentation window using the device and keep calling CheckDeviceState until S_OK or S_PRESENT_MODE_CHANGED returns.";
        D3DCASE(D3DERR_UNSUPPORTEDOVERLAY)
        "The device does not support overlay for the specified size or display mode.";
        D3DCASE(D3DERR_UNSUPPORTEDOVERLAYFORMAT)
        "The device does not support overlay for the specified surface format.";
        D3DCASE(D3DERR_CANNOTPROTECTCONTENT)
        "The specified content cannot be protected.";
        D3DCASE(D3DERR_UNSUPPORTEDCRYPTO)
        "The specified cryptographic algorithm is not supported.";
        D3DCASE(D3DERR_PRESENT_STATISTICS_DISJOINT)
        "The present statistics have no orderly sequence.";
    default:
    {
        static char text[1024] = {};
        _snprintf(text, sizeof(text), "unknown D3D9 error (%08X)\n", (unsigned)hr);
        return text;
    }
    }
#undef D3DCASE
}

//------------------------------------------------------------------------------

D3DFORMAT DX9_TextureFormat(TextureFormat format)
{
    switch (format)
    {
    case TEXTURE_FORMAT_R8G8B8A8:
        return D3DFMT_A8R8G8B8;
    case TEXTURE_FORMAT_R8G8B8X8:
        return D3DFMT_X8R8G8B8;

    case TEXTURE_FORMAT_R5G5B5A1:
        return D3DFMT_A1R5G5B5;
    case TEXTURE_FORMAT_R5G6B5:
        return D3DFMT_R5G6B5;

    case TEXTURE_FORMAT_R4G4B4A4:
        return D3DFMT_A4R4G4B4;

    case TEXTURE_FORMAT_A16R16G16B16:
        return D3DFMT_A16B16G16R16F;
    case TEXTURE_FORMAT_A32R32G32B32:
        return D3DFMT_A32B32G32R32F;

    case TEXTURE_FORMAT_R8:
        return D3DFMT_A8;
    case TEXTURE_FORMAT_R16:
        return D3DFMT_R16F;

    case TEXTURE_FORMAT_DXT1:
        return D3DFMT_DXT1;
    case TEXTURE_FORMAT_DXT3:
        return D3DFMT_DXT3;
    case TEXTURE_FORMAT_DXT5:
        return D3DFMT_DXT5;

    case TEXTURE_FORMAT_PVRTC2_4BPP_RGB:
    case TEXTURE_FORMAT_PVRTC2_4BPP_RGBA:
    case TEXTURE_FORMAT_PVRTC2_2BPP_RGB:
    case TEXTURE_FORMAT_PVRTC2_2BPP_RGBA:
        return D3DFMT_UNKNOWN;

    case TEXTURE_FORMAT_ATC_RGB:
    case TEXTURE_FORMAT_ATC_RGBA_EXPLICIT:
    case TEXTURE_FORMAT_ATC_RGBA_INTERPOLATED:
        return D3DFMT_UNKNOWN;

    case TEXTURE_FORMAT_ETC1:
    case TEXTURE_FORMAT_ETC2_R8G8B8:
    case TEXTURE_FORMAT_ETC2_R8G8B8A8:
    case TEXTURE_FORMAT_ETC2_R8G8B8A1:
        return D3DFMT_UNKNOWN;

    case TEXTURE_FORMAT_EAC_R11_UNSIGNED:
    case TEXTURE_FORMAT_EAC_R11_SIGNED:
    case TEXTURE_FORMAT_EAC_R11G11_UNSIGNED:
    case TEXTURE_FORMAT_EAC_R11G11_SIGNED:
        return D3DFMT_UNKNOWN;

    case TEXTURE_FORMAT_D16:
        return D3DFMT_D16;
    case TEXTURE_FORMAT_D24S8:
        return D3DFMT_D24S8;

    case TEXTURE_FORMAT_R16F:
        return D3DFMT_R16F;
    case TEXTURE_FORMAT_R32F:
        return D3DFMT_R32F;

    case TEXTURE_FORMAT_RG16F:
        return D3DFMT_G16R16F;
    case TEXTURE_FORMAT_RG32F:
        return D3DFMT_G32R32F;

    case TEXTURE_FORMAT_RGBA16F:
        return D3DFMT_A16B16G16R16F;
    case TEXTURE_FORMAT_RGBA32F:
        return D3DFMT_A32B32G32R32F;
    default:
        break;
    }

    return D3DFMT_UNKNOWN;
}

} // namespace rhi

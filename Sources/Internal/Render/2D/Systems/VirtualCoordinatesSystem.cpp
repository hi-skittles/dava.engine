#include "VirtualCoordinatesSystem.h"
#include "Engine/Engine.h"
#include "RenderSystem2D.h"
#include "UI/UIControlSystem.h"
#include "Render/2D/TextBlock.h"
#include "Render/2D/Sprite.h"
#include "Logger/Logger.h"

namespace DAVA
{
VirtualCoordinatesSystem::VirtualCoordinatesSystem()
{
    fixedProportions = true;
    enabledReloadResourceOnResize = false;
    wasScreenResized = false;

    desirableIndex = 0;

    virtualToPhysical = 1.f;
    physicalToVirtual = 1.f;

    EnableReloadResourceOnResize(true);
}

void VirtualCoordinatesSystem::ScreenSizeChanged()
{
    // on android sometime we catch FPU devide by 0 and several
    // calls to this method so here is simple shield
    if (physicalScreenSize.dx == 0 || physicalScreenSize.dy == 0)
    {
        return;
    }
    if (virtualScreenSize.dx == 0 || virtualScreenSize.dy == 0)
    {
        return;
    }
    wasScreenResized = false;

    virtualScreenSize = requestedVirtualScreenSize;

    float32 w, h;
    drawOffset = Vector2();
    w = float32(virtualScreenSize.dx) / float32(physicalScreenSize.dx);
    h = float32(virtualScreenSize.dy) / float32(physicalScreenSize.dy);
    float32 desD = 10000.0f;
    if (w > h)
    {
        physicalToVirtual = w;
        virtualToPhysical = float32(physicalScreenSize.dx) / float32(virtualScreenSize.dx);
        if (fixedProportions)
        {
            drawOffset.y = 0.5f * (float32(physicalScreenSize.dy) - float32(virtualScreenSize.dy) * virtualToPhysical);
        }
        else
        {
            virtualScreenSize.dy = int32(Round(physicalScreenSize.dy * physicalToVirtual));
        }

        for (int32 i = 0; i < int32(allowedSizes.size()); i++)
        {
            allowedSizes[i].toVirtual = float32(virtualScreenSize.dx) / float32(allowedSizes[i].width);
            allowedSizes[i].toPhysical = float32(physicalScreenSize.dx) / float32(allowedSizes[i].width);
            if (std::abs(allowedSizes[i].toPhysical - 1.0f) < desD)
            {
                desD = std::abs(allowedSizes[i].toPhysical - 1.0f);
                desirableIndex = i;
            }
        }
    }
    else
    {
        physicalToVirtual = h;
        virtualToPhysical = float32(physicalScreenSize.dy) / float32(virtualScreenSize.dy);
        if (fixedProportions)
        {
            drawOffset.x = 0.5f * (physicalScreenSize.dx - virtualScreenSize.dx * virtualToPhysical);
        }
        else
        {
            virtualScreenSize.dx = int32(Round(physicalScreenSize.dx * physicalToVirtual));
        }

        for (int32 i = 0; i < int32(allowedSizes.size()); i++)
        {
            allowedSizes[i].toVirtual = virtualScreenSize.dy / float32(allowedSizes[i].height);
            allowedSizes[i].toPhysical = physicalScreenSize.dy / float32(allowedSizes[i].height);
            if (std::abs(allowedSizes[i].toPhysical - 1.0f) < desD)
            {
                desD = std::abs(allowedSizes[i].toPhysical - 1.0f);
                desirableIndex = i;
            }
        }
    }

    drawOffset.y = std::floor(drawOffset.y);
    drawOffset.x = std::floor(drawOffset.x);

    fullVirtualScreenRect = Rect(-Round(drawOffset.x * physicalToVirtual),
                                 -Round(drawOffset.y * physicalToVirtual),
                                 Round((physicalScreenSize.dx - 2.f * drawOffset.x) * physicalToVirtual),
                                 Round((physicalScreenSize.dy - 2.f * drawOffset.y) * physicalToVirtual)
                                 );

    w = virtualScreenSize.dx / float32(inputAreaSize.dx);
    h = virtualScreenSize.dy / float32(inputAreaSize.dy);
    inputOffset.x = inputOffset.y = 0;
    if (w > h)
    {
        inputScaleFactor = w;
        inputOffset.y = 0.5f * (virtualScreenSize.dy - inputAreaSize.dy * inputScaleFactor);
    }
    else
    {
        inputScaleFactor = h;
        inputOffset.x = 0.5f * (virtualScreenSize.dx - inputAreaSize.dx * inputScaleFactor);
    }

    RenderSystem2D::Instance()->ScreenSizeChanged();
    GetEngineContext()->uiControlSystem->ScreenSizeChanged(GetFullScreenVirtualRect());
}

void VirtualCoordinatesSystem::EnableReloadResourceOnResize(bool enable)
{
    enabledReloadResourceOnResize = enable;
}

bool VirtualCoordinatesSystem::GetReloadResourceOnResize() const
{
    return enabledReloadResourceOnResize;
}

void VirtualCoordinatesSystem::SetPhysicalScreenSize(int32 width, int32 height)
{
    physicalScreenSize.dx = width;
    physicalScreenSize.dy = height;
    wasScreenResized = true;

    ScreenSizeChanged();
    physicalSizeChanged.Emit(physicalScreenSize);
}

void VirtualCoordinatesSystem::SetVirtualScreenSize(int32 width, int32 height)
{
    requestedVirtualScreenSize.dx = virtualScreenSize.dx = width;
    requestedVirtualScreenSize.dy = virtualScreenSize.dy = height;
    wasScreenResized = true;

    ScreenSizeChanged();
    virtualSizeChanged.Emit(virtualScreenSize);
}

void VirtualCoordinatesSystem::SetInputScreenAreaSize(int32 width, int32 height)
{
    inputAreaSize.dx = width;
    inputAreaSize.dy = height;
    wasScreenResized = true;

    ScreenSizeChanged();
    inputAreaSizeChanged.Emit(inputAreaSize);
}

void VirtualCoordinatesSystem::SetProportionsIsFixed(bool needFixed)
{
    fixedProportions = needFixed;
    wasScreenResized = true;
}

void VirtualCoordinatesSystem::RegisterAvailableResourceSize(int32 width, int32 height, const String& resourcesFolderName)
{
    VirtualCoordinatesSystem::ResourceSpaceSize newSize;
    newSize.width = width;
    newSize.height = height;
    newSize.folderName = resourcesFolderName;

    allowedSizes.push_back(newSize);

    ScreenSizeChanged();
}

void VirtualCoordinatesSystem::UnregisterAllAvailableResourceSizes()
{
    allowedSizes.clear();
}
};

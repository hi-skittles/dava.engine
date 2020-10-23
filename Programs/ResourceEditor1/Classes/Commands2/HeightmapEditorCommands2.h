#pragma once

#include "Base/BaseTypes.h"

#include "Commands2/Base/RECommand.h"
#include "Math/Rect.h"

namespace DAVA
{
class Heightmap;
class Entity;
}

class LandscapeEditorDrawSystem;
class ModifyHeightmapCommand : public RECommand
{
public:
    ModifyHeightmapCommand(LandscapeEditorDrawSystem* drawSystem, DAVA::Heightmap* originalHeightmap, const DAVA::Rect& updatedRect);
    ~ModifyHeightmapCommand() override;

private:
    void Redo() override;
    void Undo() override;

    DAVA::uint16* GetHeightmapRegion(DAVA::Heightmap* heightmap);
    void ApplyHeightmapRegion(DAVA::uint16* region);

private:
    LandscapeEditorDrawSystem* drawSystem = nullptr;

    DAVA::uint16* undoRegion = nullptr;
    DAVA::uint16* redoRegion = nullptr;
    DAVA::Rect updatedRect;
};

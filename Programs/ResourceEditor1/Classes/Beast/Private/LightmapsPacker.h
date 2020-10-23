#pragma once

#if defined(__DAVAENGINE_BEAST__)

#include "SpriteResourcesPacker.h"

#include <Beast/LightmapAtlasingData.h>

class LightmapsPacker : public SpriteResourcesPacker
{
public:
    void CreateDescriptors();
    void ParseSpriteDescriptors();
    DAVA::Vector<Beast::LightmapAtlasingData>* GetAtlasingData();

private:
    DAVA::Vector2 GetTextureSize(const DAVA::FilePath& filePath);

private:
    DAVA::Vector<Beast::LightmapAtlasingData> atlasingData;
};

#endif //#if defined(__DAVAENGINE_BEAST__)

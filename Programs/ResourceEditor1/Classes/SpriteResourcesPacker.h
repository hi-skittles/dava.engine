#pragma once

#include "FileSystem/FilePath.h"
#include "Render/RenderBase.h"

class SpriteResourcesPacker
{
public:
    virtual ~SpriteResourcesPacker();

    void SetInputDir(const DAVA::FilePath& inputDir);
    void SetOutputDir(const DAVA::FilePath& outputDir);

    void PackLightmaps(DAVA::eGPUFamily gpu);
    void PackTextures(DAVA::eGPUFamily gpu);

protected:
    void PerformPack(bool isLightmapPacking, DAVA::eGPUFamily gpu);

    DAVA::FilePath inputDir;
    DAVA::FilePath outputDir;
};

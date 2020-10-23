#include "FileSystem/FileSystem.h"

#include "Platform/DeviceInfo.h"

#include <TexturePacker/ResourcePacker2D.h>
#include "SpriteResourcesPacker.h"

SpriteResourcesPacker::~SpriteResourcesPacker()
{
}

void SpriteResourcesPacker::SetInputDir(const DAVA::FilePath& _inputDir)
{
    DVASSERT(_inputDir.IsDirectoryPathname());
    inputDir = _inputDir;
}

void SpriteResourcesPacker::SetOutputDir(const DAVA::FilePath& _outputDir)
{
    DVASSERT(_outputDir.IsDirectoryPathname());
    outputDir = _outputDir;
}

void SpriteResourcesPacker::PackLightmaps(DAVA::eGPUFamily gpu)
{
    return PerformPack(true, gpu);
}

void SpriteResourcesPacker::PackTextures(DAVA::eGPUFamily gpu)
{
    return PerformPack(false, gpu);
}

void SpriteResourcesPacker::PerformPack(bool isLightmapPacking, DAVA::eGPUFamily gpu)
{
    DAVA::FileSystem::Instance()->CreateDirectory(outputDir, true);

    DAVA::ResourcePacker2D resourcePacker;

    resourcePacker.forceRepack = true;
    resourcePacker.InitFolders(inputDir, outputDir);
    resourcePacker.isLightmapsPacking = isLightmapPacking;
    resourcePacker.PackResources({ gpu });
}

#if defined(__DAVAENGINE_BEAST__)

#include "Classes/Beast/Private/LightmapsPacker.h"

#include "Classes/Qt/Main/QtUtils.h"

void LightmapsPacker::ParseSpriteDescriptors()
{
    DAVA::FileList* fileList = new DAVA::FileList(outputDir);

    DAVA::UnorderedMap<DAVA::String, DAVA::Vector2> texturesSize;

    DAVA::int32 itemsCount = fileList->GetCount();
    for (DAVA::int32 i = 0; i < itemsCount; ++i)
    {
        const DAVA::FilePath& filePath = fileList->GetPathname(i);
        if (fileList->IsDirectory(i) || !filePath.IsEqualToExtension(".txt"))
        {
            continue;
        }

        Beast::LightmapAtlasingData data;

        data.meshInstanceName = filePath.GetBasename();

        DAVA::File* file = DAVA::File::Create(filePath, DAVA::File::OPEN | DAVA::File::READ);

        DAVA::char8 buf[512] = {};
        file->ReadLine(buf, sizeof(buf)); //textures count

        DAVA::uint32 readSize = file->ReadLine(buf, sizeof(buf)); //texture name
        DAVA::FilePath originalTextureName = outputDir + DAVA::String(buf, readSize);
        data.textureName = originalTextureName;

        file->ReadLine(buf, sizeof(buf)); //image size
        file->ReadLine(buf, sizeof(buf)); //frames count
        file->ReadLine(buf, sizeof(buf)); //frame rect

        DAVA::int32 x, y, dx, dy, unused0, unused1, unused2;
        sscanf(buf, "%d %d %d %d %d %d %d", &x, &y, &dx, &dy, &unused0, &unused1, &unused2);

        DAVA::String absoluteFileName = originalTextureName.GetAbsolutePathname();
        if (texturesSize.count(absoluteFileName) == 0)
        {
            texturesSize.emplace(absoluteFileName, GetTextureSize(originalTextureName));
        }

        DAVA::Vector2 textureSize = texturesSize[absoluteFileName];
        data.uvOffset = DAVA::Vector2(static_cast<DAVA::float32>(x) / textureSize.x, static_cast<DAVA::float32>(y) / textureSize.y);
        data.uvScale = DAVA::Vector2(static_cast<DAVA::float32>(dx) / textureSize.x, static_cast<DAVA::float32>(dy) / textureSize.y);

        file->Release();

        atlasingData.push_back(data);

        DAVA::FileSystem::Instance()->DeleteFile(filePath);
    }

    fileList->Release();
}

DAVA::Vector2 LightmapsPacker::GetTextureSize(const DAVA::FilePath& filePath)
{
    DAVA::Vector2 ret;

    DAVA::FilePath sourceTexturePathname = DAVA::FilePath::CreateWithNewExtension(filePath, DAVA::TextureDescriptor::GetLightmapTextureExtension());

    DAVA::ScopedPtr<DAVA::Image> image(DAVA::ImageSystem::LoadSingleMip(sourceTexturePathname));
    if (image)
    {
        ret.x = static_cast<DAVA::float32>(image->GetWidth());
        ret.y = static_cast<DAVA::float32>(image->GetHeight());
    }

    return ret;
}

DAVA::Vector<Beast::LightmapAtlasingData>* LightmapsPacker::GetAtlasingData()
{
    return &atlasingData;
}

void LightmapsPacker::CreateDescriptors()
{
    DAVA::FileList* fileList = new DAVA::FileList(outputDir);

    DAVA::int32 itemsCount = fileList->GetCount();
    for (DAVA::int32 i = 0; i < itemsCount; ++i)
    {
        const DAVA::FilePath& filePath = fileList->GetPathname(i);
        if (fileList->IsDirectory(i) || !filePath.IsEqualToExtension(".png"))
        {
            continue;
        }

        DAVA::TextureDescriptor* descriptor = new DAVA::TextureDescriptor();
        descriptor->Save(DAVA::TextureDescriptor::GetDescriptorPathname(filePath));
        delete descriptor;
    }

    fileList->Release();
}

#endif //#if defined(__DAVAENGINE_BEAST__)

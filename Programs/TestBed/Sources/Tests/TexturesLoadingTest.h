#pragma once

#include "Infrastructure/BaseScreen.h"
#include <Base/RefPtr.h>

namespace DAVA
{
class UITextComponent;
class Texture;
}

class TestBed;
class TexturesLoadingTest : public BaseScreen
{
public:
    TexturesLoadingTest(TestBed& app);

protected:
    struct TexturePaths
    {
        DAVA::FilePath descriptorPath;
        DAVA::FilePath texturePath;
    };
    using TextureHolder = DAVA::RefPtr<DAVA::Texture>;

    void LoadResources() override;
    void UnloadResources() override;

    DAVA::int64 LoadAtlas();
    DAVA::int64 LoadSmallFiles();
    DAVA::int64 LoadSmallFilesTwoThread();
    DAVA::int64 LoadSmallFilesBytes();

    DAVA::int64 RemoveFiles();
    DAVA::int64 CreateFiles(bool generateNoiseImage);

    void GenerateTestFilePaths();
    void CreateWebPFile(const DAVA::FilePath& imagePath, DAVA::int32 w, DAVA::int32 h) const;
    void CreateTextureDescriptorFile(const DAVA::FilePath& descriptorPath, DAVA::int32 w, DAVA::int32 h) const;

    TextureHolder LoadTexture(const DAVA::FilePath& path) const;
    void ProfileFileIO(const DAVA::FilePath& path);
    bool IsFilesPrepared() const;

    DAVA::UITextComponent* statusText = nullptr;

    DAVA::Vector<TexturePaths> largeFiles;
    DAVA::Vector<TexturePaths> smallFiles;
    DAVA::FilePath workingDir;

    DAVA::Vector<DAVA::int8> tmpReadBuffer;
};

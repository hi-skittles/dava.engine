#include "UnitTests/UnitTests.h"

#include "Base/BaseTypes.h"
#include "FileSystem/FileSystem.h"
#include "FileSystem/FilePath.h"
#include "Math/Math2D.h"
#include "Render/Image/LibPVRHelper.h"
#include "Render/Image/LibDdsHelper.h"
#include "Render/Image/Image.h"
#include "Render/Image/ImageSystem.h"
#include "Render/RenderBase.h"
#include "Render/Texture.h"
#include "Utils/CRC32.h"

#include "Render/Image/ImageConvert.h"

using namespace DAVA;

namespace ISTLocal
{
void PrepareWorkingFolder(const FilePath& folder)
{
    FileSystem::Instance()->DeleteDirectory(folder, true);
    FileSystem::Instance()->CreateDirectory(folder, true);
}

void ReleaseImages(Vector<Image*>& imageSet)
{
    for (Image* image : imageSet)
    {
        SafeRelease(image);
    }
    imageSet.clear();
}

struct TestData
{
    FilePath path;
    uint32 width;
    uint32 height;
    uint32 mipmapsCount;
    int32 fromMipmap;
    uint32 faceCount;
    PixelFormat format;
};
}

DAVA_TESTCLASS (ImageSystemTest)
{
    void LoadSaveAndCheck(ISTLocal::TestData loadingData)
    {
        const FilePath outFolderPathname = "~doc:/TestData/ImageSystemTest/";
        ISTLocal::PrepareWorkingFolder(outFolderPathname);

        String errorMessage = loadingData.path.GetStringValue();
        TEST_VERIFY_WITH_MESSAGE(loadingData.faceCount == 1, errorMessage);

        ImageSystem::LoadingParams params;
        params.minimalWidth = Texture::MINIMAL_WIDTH;
        params.minimalHeight = Texture::MINIMAL_HEIGHT;

        if (loadingData.width < params.minimalWidth)
        {
            loadingData.width = params.minimalWidth;
            loadingData.height = params.minimalHeight;
            loadingData.mipmapsCount = FastLog2(params.minimalWidth) + 1;
        }

        Vector<Image*> imageSet;
        SCOPE_EXIT
        {
            ISTLocal::ReleaseImages(imageSet);
        };

        { // Load images
            ScopedPtr<File> infile(File::Create(loadingData.path, File::OPEN | File::READ));
            TEST_VERIFY_WITH_MESSAGE(infile, errorMessage);
            params.baseMipmap = loadingData.fromMipmap;
            params.firstMipmapIndex = 0;
            eErrorCode loadCode = ImageSystem::Load(infile, imageSet, params);
            TEST_VERIFY_WITH_MESSAGE(eErrorCode::SUCCESS == loadCode, errorMessage);

            // test loaded images
            bool loaded = imageSet.size() == loadingData.mipmapsCount;
            TEST_VERIFY_WITH_MESSAGE(loaded, errorMessage);
            if (loaded && loadingData.mipmapsCount > 0)
            {
                Image* zeroMipMap = imageSet[0];
                TEST_VERIFY_WITH_MESSAGE(zeroMipMap->width >= params.minimalWidth, errorMessage);
                TEST_VERIFY_WITH_MESSAGE(zeroMipMap->height >= params.minimalHeight, errorMessage);

                for (uint32 mip = 0; mip < loadingData.mipmapsCount; ++mip)
                {
                    Image* mipImage = imageSet[mip];
                    TEST_VERIFY_WITH_MESSAGE(mipImage->mipmapLevel == mip, errorMessage);
                    TEST_VERIFY_WITH_MESSAGE(mipImage->cubeFaceID == Texture::INVALID_CUBEMAP_FACE, errorMessage);
                    TEST_VERIFY_WITH_MESSAGE(mipImage->width == (loadingData.width >> mip), errorMessage);
                    TEST_VERIFY_WITH_MESSAGE(mipImage->height == (loadingData.height >> mip), errorMessage);
                    TEST_VERIFY_WITH_MESSAGE(mipImage->format == loadingData.format, errorMessage);
                }
            }
        }

        { //Save
            FilePath savePath(loadingData.path);
            savePath.ReplaceDirectory(outFolderPathname);

            eErrorCode saveCode = ImageSystem::Save(savePath, imageSet, loadingData.format);
            TEST_VERIFY_WITH_MESSAGE(eErrorCode::SUCCESS == saveCode, errorMessage);

            Vector<Image*> reLoadedImageSet;
            SCOPE_EXIT
            {
                ISTLocal::ReleaseImages(reLoadedImageSet);
            };

            { // Load saved images
                ScopedPtr<File> infile(File::Create(savePath, File::OPEN | File::READ));
                eErrorCode loadCode = ImageSystem::Load(infile, reLoadedImageSet);
                TEST_VERIFY_WITH_MESSAGE(eErrorCode::SUCCESS == loadCode, errorMessage);
            }

            bool mipsCountEqual = (reLoadedImageSet.size() == imageSet.size());
            TEST_VERIFY_WITH_MESSAGE(mipsCountEqual, errorMessage);
            if (mipsCountEqual)
            {
                for (uint32 mip = 0; mip < loadingData.mipmapsCount; ++mip)
                {
                    TEST_VERIFY_WITH_MESSAGE(imageSet[mip]->width == reLoadedImageSet[mip]->width, errorMessage);
                    TEST_VERIFY_WITH_MESSAGE(imageSet[mip]->height == reLoadedImageSet[mip]->height, errorMessage);
                    TEST_VERIFY_WITH_MESSAGE(imageSet[mip]->format == reLoadedImageSet[mip]->format, errorMessage);
                    TEST_VERIFY_WITH_MESSAGE(imageSet[mip]->dataSize == reLoadedImageSet[mip]->dataSize, errorMessage);
                    TEST_VERIFY_WITH_MESSAGE(Memcmp(imageSet[mip]->data, reLoadedImageSet[mip]->data, reLoadedImageSet[mip]->dataSize) == 0, errorMessage);
                    TEST_VERIFY_WITH_MESSAGE(imageSet[mip]->mipmapLevel == reLoadedImageSet[mip]->mipmapLevel, errorMessage);
                    TEST_VERIFY_WITH_MESSAGE(imageSet[mip]->cubeFaceID == reLoadedImageSet[mip]->cubeFaceID, errorMessage);
                }
            }
        }
    }

    DAVA_TEST (SaveLoadTest)
    {
        static Vector<ISTLocal::TestData> testData =
        {
          { "~res:/TestData/ImageSystemTest/a8.pvr", 32, 32, 6, 0, 1, PixelFormat::FORMAT_A8 },
          { "~res:/TestData/ImageSystemTest/etc1.pvr", 32, 32, 6, 0, 1, PixelFormat::FORMAT_ETC1 },
          { "~res:/TestData/ImageSystemTest/pvr2.pvr", 32, 32, 6, 0, 1, PixelFormat::FORMAT_PVR2 },
          { "~res:/TestData/ImageSystemTest/pvr4.pvr", 32, 32, 6, 0, 1, PixelFormat::FORMAT_PVR4 },
          { "~res:/TestData/ImageSystemTest/rgb565.pvr", 32, 32, 6, 0, 1, PixelFormat::FORMAT_RGB565 },
          { "~res:/TestData/ImageSystemTest/rgb888.pvr", 32, 32, 6, 0, 1, PixelFormat::FORMAT_RGB888 },
          { "~res:/TestData/ImageSystemTest/rgba4444.pvr", 32, 32, 6, 0, 1, PixelFormat::FORMAT_RGBA4444 },
          { "~res:/TestData/ImageSystemTest/rgba5551.pvr", 32, 32, 6, 0, 1, PixelFormat::FORMAT_RGBA5551 },
          { "~res:/TestData/ImageSystemTest/rgba8888.pvr", 32, 32, 6, 0, 1, PixelFormat::FORMAT_RGBA8888 },
          { "~res:/TestData/ImageSystemTest/rgba16f.pvr", 32, 32, 6, 0, 1, PixelFormat::FORMAT_RGBA16F },
          { "~res:/TestData/ImageSystemTest/rgba32f.pvr", 32, 32, 6, 0, 1, PixelFormat::FORMAT_RGBA32F },
          { "~res:/TestData/ImageSystemTest/rgba16f.dds", 32, 32, 6, 0, 1, PixelFormat::FORMAT_RGBA16F },
          { "~res:/TestData/ImageSystemTest/rgba32f.dds", 32, 32, 6, 0, 1, PixelFormat::FORMAT_RGBA32F },
          { "~res:/TestData/ImageSystemTest/dxt1.dds", 32, 32, 6, 0, 1, PixelFormat::FORMAT_DXT1 },
          { "~res:/TestData/ImageSystemTest/dxt1a.dds", 32, 32, 6, 0, 1, PixelFormat::FORMAT_DXT1 },
          { "~res:/TestData/ImageSystemTest/dxt3.dds", 32, 32, 6, 0, 1, PixelFormat::FORMAT_DXT3 },
          { "~res:/TestData/ImageSystemTest/dxt5.dds", 32, 32, 6, 0, 1, PixelFormat::FORMAT_DXT5 },
          { "~res:/TestData/ImageSystemTest/dxt5nm.dds", 32, 32, 6, 0, 1, PixelFormat::FORMAT_DXT5 },
          { "~res:/TestData/ImageSystemTest/atcrgb.dds", 32, 32, 6, 0, 1, PixelFormat::FORMAT_ATC_RGB },
          { "~res:/TestData/ImageSystemTest/atcrgbae.dds", 32, 32, 6, 0, 1, PixelFormat::FORMAT_ATC_RGBA_EXPLICIT_ALPHA },
          { "~res:/TestData/ImageSystemTest/atcrgbai.dds", 32, 32, 6, 0, 1, PixelFormat::FORMAT_ATC_RGBA_INTERPOLATED_ALPHA },

          { "~res:/TestData/ImageSystemTest/pvr4_zeromip.pvr", 32, 32, 1, 0, 1, PixelFormat::FORMAT_PVR4 },
          { "~res:/TestData/ImageSystemTest/dxt5_zeromip.dds", 32, 32, 1, 0, 1, PixelFormat::FORMAT_DXT5 },

          { "~res:/TestData/ImageSystemTest/a8.pvr", 16, 16, 5, 1, 1, PixelFormat::FORMAT_A8 },
          { "~res:/TestData/ImageSystemTest/dxt3.dds", 16, 16, 5, 1, 1, PixelFormat::FORMAT_DXT3 },
          { "~res:/TestData/ImageSystemTest/atcrgb.dds", 16, 16, 5, 1, 1, PixelFormat::FORMAT_ATC_RGB },

          { "~res:/TestData/ImageSystemTest/etc1.pvr", 8, 8, 4, 2, 1, PixelFormat::FORMAT_ETC1 },
          { "~res:/TestData/ImageSystemTest/pvr2.pvr", 4, 4, 3, 3, 1, PixelFormat::FORMAT_PVR2 },
          { "~res:/TestData/ImageSystemTest/dxt5.dds", 4, 4, 3, 3, 1, PixelFormat::FORMAT_DXT5 },
          { "~res:/TestData/ImageSystemTest/atcrgbae.dds", 4, 4, 3, 3, 1, PixelFormat::FORMAT_ATC_RGBA_EXPLICIT_ALPHA },

          { "~res:/TestData/ImageSystemTest/pvr4.pvr", 2, 2, 2, 4, 1, PixelFormat::FORMAT_PVR4 },
          { "~res:/TestData/ImageSystemTest/rgb565.pvr", 1, 1, 1, 5, 1, PixelFormat::FORMAT_RGB565 },
        };

        for (const ISTLocal::TestData& td : testData)
        {
            TEST_VERIFY(td.width == td.height);
            LoadSaveAndCheck(td);
        }
    }

    void LoadSaveAndCheckCubemap(ISTLocal::TestData loadingData)
    {
        const FilePath outFolderPathname = "~doc:/TestData/ImageSystemTest/";
        ISTLocal::PrepareWorkingFolder(outFolderPathname);

        String errorMessage = loadingData.path.GetStringValue();
        TEST_VERIFY_WITH_MESSAGE(loadingData.faceCount == Texture::CUBE_FACE_COUNT, errorMessage);

        ImageSystem::LoadingParams params;
        params.minimalWidth = Texture::MINIMAL_WIDTH;
        params.minimalHeight = Texture::MINIMAL_HEIGHT;

        Vector<Image*> imageSet;
        SCOPE_EXIT
        {
            ISTLocal::ReleaseImages(imageSet);
        };

        Vector<Vector<Image*>> cubeImageSet(Texture::CUBE_FACE_COUNT); // [face][mip]
        for (uint32 face = 0; face < Texture::CUBE_FACE_COUNT; ++face)
        {
            cubeImageSet[face].resize(loadingData.mipmapsCount);
        }

        { // Load
            ScopedPtr<File> infile(File::Create(loadingData.path, File::OPEN | File::READ));
            params.baseMipmap = loadingData.fromMipmap;
            params.firstMipmapIndex = 0;

            eErrorCode loadCode = ImageSystem::Load(infile, imageSet, params);
            TEST_VERIFY_WITH_MESSAGE(eErrorCode::SUCCESS == loadCode, errorMessage);

            bool loaded = imageSet.size() == loadingData.mipmapsCount * Texture::CUBE_FACE_COUNT;
            TEST_VERIFY_WITH_MESSAGE(loaded, errorMessage);
            if (loaded)
            {
                for (Image* image : imageSet)
                {
                    TEST_VERIFY_WITH_MESSAGE(image->mipmapLevel < loadingData.mipmapsCount, errorMessage);
                    TEST_VERIFY_WITH_MESSAGE(image->cubeFaceID != Texture::INVALID_CUBEMAP_FACE, errorMessage);

                    cubeImageSet[image->cubeFaceID][image->mipmapLevel] = image;
                }

                for (uint32 face = 0; face < Texture::CUBE_FACE_COUNT; ++face)
                {
                    for (uint32 mip = 0; mip < loadingData.mipmapsCount; ++mip)
                    {
                        Image* image = cubeImageSet[face][mip];

                        TEST_VERIFY_WITH_MESSAGE(image->mipmapLevel == mip, errorMessage);
                        TEST_VERIFY_WITH_MESSAGE(image->cubeFaceID == face, errorMessage);

                        TEST_VERIFY_WITH_MESSAGE(image->width == (loadingData.width >> mip), errorMessage);
                        TEST_VERIFY_WITH_MESSAGE(image->height == (loadingData.height >> mip), errorMessage);
                        TEST_VERIFY_WITH_MESSAGE(image->format == loadingData.format, errorMessage);
                    }
                }
            }
        }

        { //Save
            FilePath savePath(loadingData.path);
            savePath.ReplaceDirectory(outFolderPathname);

            eErrorCode saveCode = ImageSystem::SaveAsCubeMap(savePath, cubeImageSet, loadingData.format);
            TEST_VERIFY_WITH_MESSAGE(eErrorCode::SUCCESS == saveCode, errorMessage);

            Vector<Image*> reLoadedImageSet;
            SCOPE_EXIT
            {
                ISTLocal::ReleaseImages(reLoadedImageSet);
            };

            { // Load saved images
                ScopedPtr<File> infile(File::Create(savePath, File::OPEN | File::READ));
                eErrorCode loadCode = ImageSystem::Load(infile, reLoadedImageSet);
                TEST_VERIFY_WITH_MESSAGE(eErrorCode::SUCCESS == loadCode, errorMessage);
            }

            bool sizeEqual = (reLoadedImageSet.size() == imageSet.size());
            TEST_VERIFY_WITH_MESSAGE(sizeEqual, errorMessage);
            if (sizeEqual)
            {
                uint32 count = static_cast<uint32>(imageSet.size());
                for (uint32 i = 0; i < count; ++i)
                {
                    TEST_VERIFY_WITH_MESSAGE(imageSet[i]->width == reLoadedImageSet[i]->width, errorMessage);
                    TEST_VERIFY_WITH_MESSAGE(imageSet[i]->height == reLoadedImageSet[i]->height, errorMessage);
                    TEST_VERIFY_WITH_MESSAGE(imageSet[i]->format == reLoadedImageSet[i]->format, errorMessage);
                    TEST_VERIFY_WITH_MESSAGE(imageSet[i]->dataSize == reLoadedImageSet[i]->dataSize, errorMessage);
                    TEST_VERIFY_WITH_MESSAGE(Memcmp(imageSet[i]->data, reLoadedImageSet[i]->data, reLoadedImageSet[i]->dataSize) == 0, errorMessage);
                    TEST_VERIFY_WITH_MESSAGE(imageSet[i]->mipmapLevel == reLoadedImageSet[i]->mipmapLevel, errorMessage);
                    TEST_VERIFY_WITH_MESSAGE(imageSet[i]->cubeFaceID == reLoadedImageSet[i]->cubeFaceID, errorMessage);
                }
            }
        }
    }

    DAVA_TEST (SaveLoadCubemapTest)
    {
        static Vector<ISTLocal::TestData> testData =
        {
          { "~res:/TestData/ImageSystemTest/pvr2_cube.pvr", 32, 32, 6, 0, Texture::CUBE_FACE_COUNT, PixelFormat::FORMAT_PVR2 },
          { "~res:/TestData/ImageSystemTest/dxt5_cube.dds", 32, 32, 6, 0, Texture::CUBE_FACE_COUNT, PixelFormat::FORMAT_DXT5 },
        };

        for (const ISTLocal::TestData& td : testData)
        {
            TEST_VERIFY(td.width == td.height);
            LoadSaveAndCheckCubemap(td);
        }
    }

    DAVA_TEST (ImageInfoTest)
    {
        static Vector<ISTLocal::TestData> testData =
        {
          { "~res:/TestData/ImageSystemTest/a8.pvr", 32, 32, 6, 0, 1, PixelFormat::FORMAT_A8 },
          { "~res:/TestData/ImageSystemTest/etc1.pvr", 32, 32, 6, 0, 1, PixelFormat::FORMAT_ETC1 },
          { "~res:/TestData/ImageSystemTest/pvr2.pvr", 32, 32, 6, 0, 1, PixelFormat::FORMAT_PVR2 },
          { "~res:/TestData/ImageSystemTest/pvr4.pvr", 32, 32, 6, 0, 1, PixelFormat::FORMAT_PVR4 },
          { "~res:/TestData/ImageSystemTest/rgb565.pvr", 32, 32, 6, 0, 1, PixelFormat::FORMAT_RGB565 },
          { "~res:/TestData/ImageSystemTest/rgb888.pvr", 32, 32, 6, 0, 1, PixelFormat::FORMAT_RGB888 },
          { "~res:/TestData/ImageSystemTest/rgba4444.pvr", 32, 32, 6, 0, 1, PixelFormat::FORMAT_RGBA4444 },
          { "~res:/TestData/ImageSystemTest/rgba5551.pvr", 32, 32, 6, 0, 1, PixelFormat::FORMAT_RGBA5551 },
          { "~res:/TestData/ImageSystemTest/rgba8888.pvr", 32, 32, 6, 0, 1, PixelFormat::FORMAT_RGBA8888 },
          { "~res:/TestData/ImageSystemTest/rgba16f.pvr", 32, 32, 6, 0, 1, PixelFormat::FORMAT_RGBA16F },
          { "~res:/TestData/ImageSystemTest/rgba32f.pvr", 32, 32, 6, 0, 1, PixelFormat::FORMAT_RGBA32F },
          { "~res:/TestData/ImageSystemTest/rgba16f.dds", 32, 32, 6, 0, 1, PixelFormat::FORMAT_RGBA16F },
          { "~res:/TestData/ImageSystemTest/rgba32f.dds", 32, 32, 6, 0, 1, PixelFormat::FORMAT_RGBA32F },
          { "~res:/TestData/ImageSystemTest/dxt1.dds", 32, 32, 6, 0, 1, PixelFormat::FORMAT_DXT1 },
          { "~res:/TestData/ImageSystemTest/dxt1a.dds", 32, 32, 6, 0, 1, PixelFormat::FORMAT_DXT1 },
          { "~res:/TestData/ImageSystemTest/dxt3.dds", 32, 32, 6, 0, 1, PixelFormat::FORMAT_DXT3 },
          { "~res:/TestData/ImageSystemTest/dxt5.dds", 32, 32, 6, 0, 1, PixelFormat::FORMAT_DXT5 },
          { "~res:/TestData/ImageSystemTest/dxt5nm.dds", 32, 32, 6, 0, 1, PixelFormat::FORMAT_DXT5 },
          { "~res:/TestData/ImageSystemTest/atcrgb.dds", 32, 32, 6, 0, 1, PixelFormat::FORMAT_ATC_RGB },
          { "~res:/TestData/ImageSystemTest/atcrgbae.dds", 32, 32, 6, 0, 1, PixelFormat::FORMAT_ATC_RGBA_EXPLICIT_ALPHA },
          { "~res:/TestData/ImageSystemTest/atcrgbai.dds", 32, 32, 6, 0, 1, PixelFormat::FORMAT_ATC_RGBA_INTERPOLATED_ALPHA },
          { "~res:/TestData/ImageSystemTest/pvr2_cube.pvr", 32, 32, 6, 0, Texture::CUBE_FACE_COUNT, PixelFormat::FORMAT_PVR2 },
          { "~res:/TestData/ImageSystemTest/dxt5_cube.dds", 32, 32, 6, 0, Texture::CUBE_FACE_COUNT, PixelFormat::FORMAT_DXT5 },
          { "~res:/TestData/ImageSystemTest/pvr4_zeromip.pvr", 32, 32, 1, 0, 1, PixelFormat::FORMAT_PVR4 },
          { "~res:/TestData/ImageSystemTest/dxt5_zeromip.dds", 32, 32, 1, 0, 1, PixelFormat::FORMAT_DXT5 },
        };

        for (const ISTLocal::TestData& td : testData)
        {
            ImageInfo info = ImageSystem::GetImageInfo(td.path);
            TEST_VERIFY(td.width == info.width);
            TEST_VERIFY(td.height == info.height);
            TEST_VERIFY(td.format == info.format);
            TEST_VERIFY(td.mipmapsCount == info.mipmapsCount);

            uint32 dataSize = 0;
            for (uint32 face = 0; face < td.faceCount; ++face)
            {
                for (uint32 mip = 0; mip < td.mipmapsCount; ++mip)
                {
                    dataSize += ImageUtils::GetSizeInBytes(td.width >> mip, td.height >> mip, td.format);
                }
            }
            TEST_VERIFY(dataSize == info.dataSize);
            TEST_VERIFY(info.faceCount == td.faceCount);
        }
    }

    void AddGetCRCFromMetaData(const FilePath& sourceImagePath, Function<bool(const FilePath&)> addCrcFn, Function<uint32(const FilePath&)> getCrcFn)
    {
        const FilePath outFolderPathname = "~doc:/TestData/ImageSystemTest/";
        ISTLocal::PrepareWorkingFolder(outFolderPathname);

        String errorMessage = sourceImagePath.GetStringValue();

        FilePath savePath(sourceImagePath);
        savePath.ReplaceDirectory(outFolderPathname);

        TEST_VERIFY_WITH_MESSAGE(FileSystem::Instance()->CopyFile(sourceImagePath, savePath), errorMessage);

        TEST_VERIFY_WITH_MESSAGE(getCrcFn(savePath) == 0, errorMessage);

        const uint32 crc = CRC32::ForFile(savePath);
        TEST_VERIFY_WITH_MESSAGE(addCrcFn(savePath), errorMessage);
        TEST_VERIFY_WITH_MESSAGE(getCrcFn(savePath) == crc, errorMessage);
    }

    DAVA_TEST (CRCTest)
    {
        AddGetCRCFromMetaData("~res:/TestData/ImageSystemTest/pvr4.pvr", &LibPVRHelper::AddCRCIntoMetaData, &LibPVRHelper::GetCRCFromMetaData);
        AddGetCRCFromMetaData("~res:/TestData/ImageSystemTest/dxt1.dds", &LibDdsHelper::AddCRCIntoMetaData, &LibDdsHelper::GetCRCFromMetaData);
    }
};

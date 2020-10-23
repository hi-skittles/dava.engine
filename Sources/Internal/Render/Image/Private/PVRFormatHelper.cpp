#include "Render/Image/Private/PVRFormatHelper.h"
#include "Render/Image/ImageConvert.h"
#include "Render/Image/Image.h"
#include "Render/PixelFormatDescriptor.h"

#include "Base/BaseTypes.h"

#include "Render/Texture.h"

#include "FileSystem/File.h"

#include "Utils/StringFormat.h"

namespace DAVA
{
PVRFile::~PVRFile()
{
    for (MetaDataBlock* block : metaDatablocks)
    {
        delete block;
    }
    metaDatablocks.clear();
}

namespace PVRFormatHelper
{
const uint32 PVRTEX3_METADATAIDENT = 0x03525650;

const uint32 METADATA_FOURCC_OFFSET = 0;
const uint32 METADATA_KEY_OFFSET = 4;
const uint32 METADATA_DATASIZE_OFFSET = 8;
const uint32 METADATA_DATA_OFFSET = 12;

const uint32 METADATA_CRC_DATA_SIZE = 4; //size for CRC32
const uint32 METADATA_CRC_SIZE = (METADATA_DATA_OFFSET + METADATA_CRC_DATA_SIZE); //size for meta data with CRC32
const uint32 METADATA_CRC_KEY = 0x5f435243; // equivalent of 'C''R''C''_'

const uint32 METADATA_CUBE_KEY = 2;
const uint32 METADATA_CUBE_SIZE = 6;

bool DetectIfNeedSwapBytes(const PVRHeaderV3* header)
{
    if ((PVRTEX_CURR_IDENT != header->u32Version) && (PVRTEX_CURR_IDENT_REV != header->u32Version))
    {
        return (PVRTIsLittleEndian() == false);
    }

    return (PVRTEX_CURR_IDENT_REV == header->u32Version);
}

uint32 GetDataSize(const Vector<Image*>& imageSet)
{
    uint32 dataSize = 0;

    for (const Image* image : imageSet)
    {
        dataSize += ImageUtils::GetSizeInBytes(image->width, image->height, image->format);
    }

    return dataSize;
}

uint32 GetDataSize(const Vector<Vector<Image*>>& imageSet)
{
    uint32 dataSize = 0;

    for (const Vector<Image*>& faceImages : imageSet)
    {
        dataSize += GetDataSize(faceImages);
    }

    return dataSize;
}

PixelFormat GetDAVAFormatFromPVR(uint64 pixelFormat, uint32 u32ChannelType)
{
    static const UnorderedMap<uint64, PixelFormat, std::hash<uint64>> mapping =
    {
      { ePVRTPF_PVRTCI_2bpp_RGB, FORMAT_PVR2 },
      { ePVRTPF_PVRTCI_2bpp_RGBA, FORMAT_PVR2 },
      { ePVRTPF_PVRTCI_4bpp_RGB, FORMAT_PVR4 },
      { ePVRTPF_PVRTCI_4bpp_RGBA, FORMAT_PVR4 },
      { ePVRTPF_PVRTCII_2bpp, FORMAT_PVR2_2 },
      { ePVRTPF_PVRTCII_4bpp, FORMAT_PVR4_2 },
      { ePVRTPF_ETC1, FORMAT_ETC1 },
      { ePVRTPF_EAC_R11, FORMAT_EAC_R11_UNSIGNED },
      { ePVRTPF_ETC2_RGB, FORMAT_ETC2_RGB },
      { ePVRTPF_ETC2_RGB_A1, FORMAT_ETC2_RGB_A1 },
      { ePVRTPF_EAC_RG11, FORMAT_EAC_RG11_UNSIGNED },
      { ePVRTPF_ETC2_RGBA, FORMAT_ETC2_RGBA },
      { PVRTGENPIXELID4('r', 'g', 'b', 'a', 8, 8, 8, 8), FORMAT_RGBA8888 },
      { PVRTGENPIXELID4('r', 'g', 'b', 'a', 5, 5, 5, 1), FORMAT_RGBA5551 },
      { PVRTGENPIXELID4('r', 'g', 'b', 'a', 4, 4, 4, 4), FORMAT_RGBA4444 },
      { PVRTGENPIXELID3('r', 'g', 'b', 8, 8, 8), FORMAT_RGB888 },
      { PVRTGENPIXELID3('r', 'g', 'b', 5, 6, 5), FORMAT_RGB565 },
      { PVRTGENPIXELID1('l', 8), FORMAT_A8 },
      { PVRTGENPIXELID1('a', 8), FORMAT_A8 },
      { PVRTGENPIXELID1('a', 16), FORMAT_A16 },
      { PVRTGENPIXELID4('r', 'g', 'b', 'a', 16, 16, 16, 16), FORMAT_RGBA16161616 },
      { PVRTGENPIXELID4('r', 'g', 'b', 'a', 32, 32, 32, 32), FORMAT_RGBA32323232 }
    };

    auto found = mapping.find(pixelFormat);
    if (found != mapping.end())
    {
        if (found->second == FORMAT_RGBA16161616 && u32ChannelType == ePVRTVarTypeFloat)
        {
            return FORMAT_RGBA16F;
        }
        if (found->second == FORMAT_RGBA32323232 && u32ChannelType == ePVRTVarTypeFloat)
        {
            return FORMAT_RGBA32F;
        }

        return found->second;
    }

    DVASSERT(false, Format("Unsupported format: %lu", pixelFormat).c_str());
    return FORMAT_INVALID;
}

uint64 GetPVRFormatFromDAVA(PixelFormat pixelFormat)
{
    static const UnorderedMap<PixelFormat, uint64, std::hash<uint8>> mapping =
    {
      { FORMAT_PVR2, ePVRTPF_PVRTCI_2bpp_RGBA },
      { FORMAT_PVR4, ePVRTPF_PVRTCI_4bpp_RGBA },
      { FORMAT_PVR2_2, ePVRTPF_PVRTCII_2bpp },
      { FORMAT_PVR4_2, ePVRTPF_PVRTCII_4bpp },

      { FORMAT_ETC1, ePVRTPF_ETC1 },
      { FORMAT_EAC_R11_UNSIGNED, ePVRTPF_EAC_R11 },
      { FORMAT_ETC2_RGB, FORMAT_ETC2_RGB },
      { FORMAT_ETC2_RGB_A1, FORMAT_ETC2_RGB_A1 },
      { FORMAT_EAC_RG11_UNSIGNED, ePVRTPF_EAC_RG11 },

      { FORMAT_ETC2_RGBA, ePVRTPF_ETC2_RGBA },
      { FORMAT_RGBA8888, PVRTGENPIXELID4('r', 'g', 'b', 'a', 8, 8, 8, 8) },
      { FORMAT_RGBA5551, PVRTGENPIXELID4('r', 'g', 'b', 'a', 5, 5, 5, 1) },
      { FORMAT_RGBA4444, PVRTGENPIXELID4('r', 'g', 'b', 'a', 4, 4, 4, 4) },
      { FORMAT_RGB888, PVRTGENPIXELID3('r', 'g', 'b', 8, 8, 8) },
      { FORMAT_RGB565, PVRTGENPIXELID3('r', 'g', 'b', 5, 6, 5) },
      { FORMAT_A8, PVRTGENPIXELID1('a', 8) },
      { FORMAT_A16, PVRTGENPIXELID1('a', 16) },
      { FORMAT_RGBA16161616, PVRTGENPIXELID4('r', 'g', 'b', 'a', 16, 16, 16, 16) },
      { FORMAT_RGBA32323232, PVRTGENPIXELID4('r', 'g', 'b', 'a', 32, 32, 32, 32) },
      { FORMAT_RGBA16F, PVRTGENPIXELID4('r', 'g', 'b', 'a', 16, 16, 16, 16) },
      { FORMAT_RGBA32F, PVRTGENPIXELID4('r', 'g', 'b', 'a', 32, 32, 32, 32) }
    };

    auto found = mapping.find(pixelFormat);
    if (found != mapping.end())
    {
        return found->second;
    }

    DVASSERT(false, Format("Unsupported format: %u", pixelFormat).c_str());
    return ePVRTPF_NumCompressedPFs;
}

uint32 GetPVRChannelType(PixelFormat pixelFormat)
{
    switch (pixelFormat)
    {
    case FORMAT_PVR2:
    case FORMAT_PVR4:
    case FORMAT_PVR2_2:
    case FORMAT_PVR4_2:
    case FORMAT_ETC1:
    case FORMAT_EAC_R11_UNSIGNED:
    case FORMAT_ETC2_RGB:
    case FORMAT_ETC2_RGB_A1:
    case FORMAT_EAC_RG11_UNSIGNED:
    case FORMAT_ETC2_RGBA:
        return ePVRTVarTypeUnsignedByteNorm;

    case FORMAT_RGBA8888:
    case FORMAT_RGB888:
    case FORMAT_A8:
    case FORMAT_RGBA16161616:
    case FORMAT_RGBA32323232:
        return ePVRTVarTypeUnsignedByteNorm;

    case FORMAT_RGBA5551:
    case FORMAT_RGBA4444:
    case FORMAT_RGB565:
    case FORMAT_A16:
        return ePVRTVarTypeUnsignedShortNorm;

    case FORMAT_RGBA16F:
    case FORMAT_RGBA32F:
        return ePVRTVarTypeFloat;

    default:
        DVASSERT(false);
        break;
    }

    return ePVRTVarTypeUnsignedByteNorm;
}

PixelFormat GetPixelFormat(const PVRHeaderV3& textureHeader)
{
    return GetDAVAFormatFromPVR(textureHeader.u64PixelFormat, textureHeader.u32ChannelType);
}

const MetaDataBlock* GetCubemapMetadata(const PVRFile& pvrFile)
{
    for (MetaDataBlock* block : pvrFile.metaDatablocks)
    {
        if (block->DevFOURCC == PVRTEX3_METADATAIDENT &&
            block->u32Key == METADATA_CUBE_KEY &&
            block->u32DataSize == METADATA_CUBE_SIZE)
        {
            return block;
        }
    }

    return nullptr;
}

void AddMetaData(PVRFile& pvrFile, MetaDataBlock* block)
{
    pvrFile.header.u32MetaDataSize += (block->u32DataSize + METADATA_DATA_OFFSET);
    pvrFile.metaDatablocks.push_back(block);
}

bool ReadMetaData(File* file, PVRFile* pvrFile)
{
    uint32 metaDataSize = pvrFile->header.u32MetaDataSize;
    while (metaDataSize != 0)
    {
        // fields from MetaDataBlock
        uint32 DevFOURCC = 0;
        uint32 u32Key = 0;
        uint32 u32DataSize = 0;
        uint8* Data = nullptr;

        uint32 readSize = file->Read(&DevFOURCC);
        if (readSize != sizeof(uint32))
        {
            return false;
        }

        readSize = file->Read(&u32Key);
        if (readSize != sizeof(uint32))
        {
            return false;
        }

        readSize = file->Read(&u32DataSize);
        if (readSize != sizeof(uint32))
        {
            return false;
        }

        if (u32DataSize != 0)
        {
            Data = new uint8[u32DataSize];
            readSize = file->Read(Data, u32DataSize);
            if (readSize != u32DataSize)
            {
                delete[] Data;
                return false;
            }
        }

        DVASSERT(metaDataSize >= u32DataSize + METADATA_DATA_OFFSET);
        metaDataSize -= (u32DataSize + METADATA_DATA_OFFSET);

        MetaDataBlock* block = new MetaDataBlock();
        block->DevFOURCC = DevFOURCC;
        block->u32Key = u32Key;
        block->u32DataSize = u32DataSize;
        block->Data = Data;

        pvrFile->metaDatablocks.push_back(block);
    }

    return true;
}

uint32 GetCubemapLayout(const PVRFile& pvrFile)
{
    uint32 layout = 0;

    const MetaDataBlock* cubeMetaData = GetCubemapMetadata(pvrFile);
    if (cubeMetaData)
    {
        for (uint32 index = 0; index < cubeMetaData->u32DataSize; ++index)
        {
            switch (cubeMetaData->Data[index])
            {
            case 'X':
            {
                layout = layout | (rhi::TEXTURE_FACE_POSITIVE_X << (index * 4));
                break;
            }

            case 'x':
            {
                layout = layout | (rhi::TEXTURE_FACE_NEGATIVE_X << (index * 4));
                break;
            }

            case 'Y':
            {
                layout = layout | (rhi::TEXTURE_FACE_POSITIVE_Y << (index * 4));
                break;
            }

            case 'y':
            {
                layout = layout | (rhi::TEXTURE_FACE_NEGATIVE_Y << (index * 4));
                break;
            }

            case 'Z':
            {
                layout = layout | (rhi::TEXTURE_FACE_POSITIVE_Z << (index * 4));
                break;
            }

            case 'z':
            {
                layout = layout | (rhi::TEXTURE_FACE_NEGATIVE_Z << (index * 4));
                break;
            }
            }
        }
    }
    else if (pvrFile.header.u32NumFaces > 1)
    {
        static uint32 faces[] = {
            rhi::TEXTURE_FACE_POSITIVE_X,
            rhi::TEXTURE_FACE_NEGATIVE_X,
            rhi::TEXTURE_FACE_POSITIVE_Y,
            rhi::TEXTURE_FACE_NEGATIVE_Y,
            rhi::TEXTURE_FACE_POSITIVE_Z,
            rhi::TEXTURE_FACE_NEGATIVE_Z
        };
        for (uint32 i = 0; i < pvrFile.header.u32NumFaces; ++i)
        {
            layout = layout | (faces[i] << (i * 4));
        }
    }

    return layout;
}

//external methods

std::unique_ptr<PVRFile> ReadFile(const FilePath& pathname, bool readMetaData, bool readData)
{
    ScopedPtr<File> file(File::Create(pathname, File::OPEN | File::READ));
    if (file)
    {
        return ReadFile(file, readMetaData, readData);
    }

    return std::unique_ptr<PVRFile>();
}

std::unique_ptr<PVRFile> ReadFile(File* file, bool readMetaData /*= false*/, bool readData /*= false*/)
{
    DVASSERT(file != nullptr);
    DVASSERT(file->GetPos() == 0);

    std::unique_ptr<PVRFile> pvrFile(new PVRFile());

    uint32 readSize = file->Read(&pvrFile->header, PVRFile::HEADER_SIZE);
    if (readSize != PVRFile::HEADER_SIZE)
    {
        Logger::Error("Can't read PVR header from %s", file->GetFilename().GetStringValue().c_str());
        return std::unique_ptr<PVRFile>();
    }

    if (DetectIfNeedSwapBytes(&pvrFile->header))
    {
        Logger::Error("Can't load PVR with swapped bytes %s", file->GetFilename().GetStringValue().c_str());
        return std::unique_ptr<PVRFile>();
    }

    if (readMetaData && pvrFile->header.u32MetaDataSize)
    {
        bool read = ReadMetaData(file, pvrFile.get());
        if (!read)
        {
            Logger::Error("Can't read metadata from PVR header from %s", file->GetFilename().GetStringValue().c_str());
            return std::unique_ptr<PVRFile>();
        }
    }
    else if (pvrFile->header.u32MetaDataSize)
    {
        file->Seek(pvrFile->header.u32MetaDataSize, File::SEEK_FROM_CURRENT);
    }

    if (readData)
    {
        uint32 compressedDataSize = static_cast<uint32>(file->GetSize() - (PVRFile::HEADER_SIZE + pvrFile->header.u32MetaDataSize));
        pvrFile->compressedData.resize(compressedDataSize);
        readSize = file->Read(pvrFile->compressedData.data(), compressedDataSize);
        if (readSize != compressedDataSize)
        {
            Logger::Error("Can't read PVR data from %s", file->GetFilename().GetStringValue().c_str());
            return std::unique_ptr<PVRFile>();
        }
    }

    return pvrFile;
}

std::unique_ptr<PVRFile> CreateHeader(const Vector<Image*>& imageSet)
{
    std::unique_ptr<PVRFile> pvrFile(new PVRFile());

    Image* zeroMip = imageSet[0];
    pvrFile->header.u64PixelFormat = GetPVRFormatFromDAVA(zeroMip->format);
    pvrFile->header.u32ChannelType = GetPVRChannelType(zeroMip->format);

    pvrFile->header.u32Width = zeroMip->width;
    pvrFile->header.u32Height = zeroMip->height;

    pvrFile->header.u32NumFaces = 1;

    DVASSERT(zeroMip->cubeFaceID == Texture::INVALID_CUBEMAP_FACE);

    pvrFile->header.u32MIPMapCount = static_cast<uint32>(imageSet.size());
    return pvrFile;
}

std::unique_ptr<PVRFile> CreateCubeHeader(const Vector<Vector<Image*>>& imageSet)
{
    std::unique_ptr<PVRFile> pvrFile(new PVRFile());

    const Vector<Image*>& zeroFaceImageSet = imageSet[0];
    Image* zeroMip = zeroFaceImageSet[0];
    pvrFile->header.u64PixelFormat = GetPVRFormatFromDAVA(zeroMip->format);
    pvrFile->header.u32ChannelType = GetPVRChannelType(zeroMip->format);

    pvrFile->header.u32Width = zeroMip->width;
    pvrFile->header.u32Height = zeroMip->height;

    pvrFile->header.u32NumFaces = static_cast<uint32>(imageSet.size());
    pvrFile->header.u32MIPMapCount = static_cast<uint32>(zeroFaceImageSet.size());

    MetaDataBlock* cubeMetaBlock = new MetaDataBlock();
    cubeMetaBlock->DevFOURCC = PVRTEX3_METADATAIDENT;
    cubeMetaBlock->u32Key = METADATA_CUBE_KEY;
    cubeMetaBlock->u32DataSize = METADATA_CUBE_SIZE;
    cubeMetaBlock->Data = new uint8[METADATA_CUBE_SIZE];
    Memcpy(cubeMetaBlock->Data, "XxYyZz", METADATA_CUBE_SIZE);
    AddMetaData(*pvrFile, cubeMetaBlock);

    return pvrFile;
}

bool LoadImages(File* infile, Vector<Image*>& imageSet, const ImageSystem::LoadingParams& loadingParams)
{
    std::unique_ptr<PVRFile> pvrFile = PVRFormatHelper::ReadFile(infile, true, false);
    if (!pvrFile)
    {
        return false;
    }

    if (pvrFile->header.u32NumSurfaces != 1)
    {
        Logger::Error("File %s has wrong surface count %d. Must be 1", infile->GetFilename().GetStringValue().c_str(), pvrFile->header.u32NumSurfaces);
        return false;
    }

    PixelFormat pxFormat = GetPixelFormat(pvrFile->header);
    if (pvrFile->header.u32Height != pvrFile->header.u32Width && (pxFormat == FORMAT_PVR2 || pxFormat == FORMAT_PVR4))
    {
        Logger::Error("Non-square textures with %s compression are unsupported. Failed to load : %s", GlobalEnumMap<PixelFormat>::Instance()->ToString(pxFormat),
                      infile->GetFilename().GetStringValue().c_str());
        return false;
    }

    if (pvrFile->header.u32MIPMapCount == 0)
    {
        Logger::Error("File %s has no mipmaps", infile->GetFilename().GetStringValue().c_str());
        return false;
    }

    uint32 fromMipMap = ImageSystem::GetBaseMipmap({ pvrFile->header.u32Width, pvrFile->header.u32Height, Min(loadingParams.baseMipmap, pvrFile->header.u32MIPMapCount - 1) }, loadingParams);

    uint32 cubemapLayout = GetCubemapLayout(*pvrFile);
    for (uint32 mip = 0; mip < pvrFile->header.u32MIPMapCount; ++mip)
    {
        for (uint32 surface = 0; surface < pvrFile->header.u32NumSurfaces; ++surface)
        {
            for (uint32 face = 0; face < pvrFile->header.u32NumFaces; ++face)
            {
                uint32 mipWidth = Max(pvrFile->header.u32Width >> mip, 1u);
                uint32 mipHeight = Max(pvrFile->header.u32Height >> mip, 1u);
                uint32 mipDataSize = ImageUtils::GetSizeInBytes(mipWidth, mipHeight, pxFormat);

                if (mip < fromMipMap)
                {
                    infile->Seek(mipDataSize, File::SEEK_FROM_CURRENT);
                }
                else
                {
                    Image* image = new Image();
                    image->width = mipWidth;
                    image->height = mipHeight;
                    image->format = pxFormat;
                    image->mipmapLevel = (mip - fromMipMap) + loadingParams.firstMipmapIndex;
                    if (cubemapLayout != 0)
                    {
                        image->cubeFaceID = (cubemapLayout & (0x0000000F << (face * 4))) >> (face * 4);
                    }

                    image->dataSize = mipDataSize;
                    image->data = new uint8[image->dataSize];
                    uint32 dz = infile->Read(image->data, image->dataSize);
                    if (dz != image->dataSize)
                    {
                        image->Release();
                        Logger::Error("Cannot read mip %d, fase %d from file", mip, face, infile->GetFilename().GetStringValue().c_str());
                        return false;
                    }

                    //TODO: this code should be re-worked with texture refactoring
                    if (image->format == PixelFormat::FORMAT_RGBA4444
                        || image->format == PixelFormat::FORMAT_RGBA5551)
                    {
                        Image* realImage = Image::Create(image->width, image->height, image->format);
                        realImage->mipmapLevel = image->mipmapLevel;
                        realImage->cubeFaceID = image->cubeFaceID;

                        if (image->format == PixelFormat::FORMAT_RGBA4444)
                        {
                            ConvertDirect<uint16, uint16, ConvertABGR4444toRGBA4444> convert;
                            convert(image->data, image->width, image->height, image->width * sizeof(uint16), realImage->data);
                        }
                        else if (image->format == PixelFormat::FORMAT_RGBA5551)
                        {
                            ConvertDirect<uint16, uint16, ConvertABGR1555toRGBA5551> convert;
                            convert(image->data, image->width, image->height, image->width * sizeof(uint16), realImage->data);
                        }

                        image->Release();
                        image = realImage;
                    }
                    //END of TODO

                    imageSet.push_back(image);
                }
            }
        }
    }

    return true;
}

bool WriteFile(const FilePath& pathname, const PVRFile& pvrFile)
{
    ScopedPtr<File> file(File::Create(pathname, File::CREATE | File::WRITE));
    if (file)
    {
        return WriteFile(file, pvrFile);
    }

    Logger::Error("Сan't open file: %s", pathname.GetStringValue().c_str());
    return false;
}

bool WriteFile(ScopedPtr<File>& file, const PVRFile& pvrFile)
{
    DVASSERT(file);

    uint32 written = file->Write(&pvrFile.header);
    DVASSERT(file->GetPos() == PVRFile::HEADER_SIZE);
    DVASSERT(written == PVRFile::HEADER_SIZE);

    for (MetaDataBlock* block : pvrFile.metaDatablocks)
    {
        file->Write(&block->DevFOURCC);
        file->Write(&block->u32Key);
        file->Write(&block->u32DataSize);
        if (block->u32DataSize != 0)
        {
            file->Write(block->Data, block->u32DataSize);
        }
    }

    DVASSERT(file->GetPos() == PVRFile::HEADER_SIZE + pvrFile.header.u32MetaDataSize);

    if (!pvrFile.compressedData.empty())
    {
        written = file->Write(pvrFile.compressedData.data(), static_cast<uint32>(pvrFile.compressedData.size()));
        DVASSERT(written == pvrFile.compressedData.size());
        DVASSERT(file->GetPos() == PVRFile::HEADER_SIZE + pvrFile.header.u32MetaDataSize + pvrFile.compressedData.size());
    }
    return true;
}

bool GetCRCFromMetaData(const PVRFile& pvrFile, uint32* outputCRC)
{
    for (MetaDataBlock* block : pvrFile.metaDatablocks)
    {
        if (block->u32Key == METADATA_CRC_KEY)
        {
            *outputCRC = *(reinterpret_cast<uint32*>(block->Data));
            return true;
        }
    }

    return false;
}

void AddCRCToMetaData(PVRFile& pvrFile, uint32 crc)
{
    //reallocate meta data buffer
    MetaDataBlock* crcMetaData = new MetaDataBlock();
    crcMetaData->DevFOURCC = PVRTEX3_METADATAIDENT;
    crcMetaData->u32Key = METADATA_CRC_KEY;
    crcMetaData->u32DataSize = METADATA_CRC_DATA_SIZE;
    crcMetaData->Data = new uint8[sizeof(uint32)];
    *(reinterpret_cast<uint32*>(crcMetaData->Data)) = crc;

    AddMetaData(pvrFile, crcMetaData);
}

} // PVRFormatHelper
} //DAVA

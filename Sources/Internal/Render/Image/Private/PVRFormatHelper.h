#pragma once

#include "Base/BaseTypes.h"
#include "Render/RenderBase.h"
#include "Render/Image/ImageSystem.h"

#if defined(__DAVAENGINE_MACOS__) || defined(__DAVAENGINE_IPHONE__)
#include <objc/objc.h>
#endif

#include "Render/Image/Private/PVRDefines.h"

namespace DAVA
{

#pragma pack(push, 4)

struct PVRHeaderV3
{
    uint32 u32Version = PVRTEX3_IDENT; //Version of the file header, used to identify it.
    uint32 u32Flags = 0; //Various format flags.
    uint64 u64PixelFormat = ePVRTPF_NumCompressedPFs; //The pixel format, 8cc value storing the 4 channel identifiers and their respective sizes.
    uint32 u32ColourSpace = ePVRTCSpacelRGB; //The Colour Space of the texture, currently either linear RGB or sRGB.
    uint32 u32ChannelType = 0; //Variable type that the channel is stored in. Supports signed/unsigned int/short/byte or float for now.
    uint32 u32Height = 1; //Height of the texture.
    uint32 u32Width = 1; //Width of the texture.
    uint32 u32Depth = 1; //Depth of the texture. (Z-slices)
    uint32 u32NumSurfaces = 1; //Number of members in a Texture Array.
    uint32 u32NumFaces = 1; //Number of faces in a Cube Map. Maybe be a value other than 6.
    uint32 u32MIPMapCount = 1; //Number of MIP Maps in the texture - NB: Includes top level.
    uint32 u32MetaDataSize = 0; //Size of the accompanying meta data.
};
#pragma pack(pop)

struct PVRFile final
{
    static const uint32 HEADER_SIZE = 52;
    ~PVRFile();

    PVRHeaderV3 header;
    Vector<MetaDataBlock*> metaDatablocks;
    Vector<uint8> compressedData;
};

class File;
class Image;

namespace PVRFormatHelper
{
std::unique_ptr<PVRFile> ReadFile(const FilePath& pathname, bool readMetaData, bool readData);
std::unique_ptr<PVRFile> ReadFile(File* file, bool readMetaData, bool readData);

std::unique_ptr<PVRFile> CreateHeader(const Vector<Image*>& imageSet);
std::unique_ptr<PVRFile> CreateCubeHeader(const Vector<Vector<Image*>>& imageSet);

bool WriteFile(const FilePath& pathname, const PVRFile& pvrFile);
bool WriteFile(ScopedPtr<File>& file, const PVRFile& pvrFile);

bool GetCRCFromMetaData(const PVRFile& pvrFile, uint32* outputCRC);
void AddCRCToMetaData(PVRFile& pvrFile, uint32 crc);

bool LoadImages(File* infile, Vector<Image*>& imageSet, const ImageSystem::LoadingParams& loadingParams);

PixelFormat GetPixelFormat(const PVRHeaderV3& textureHeader);
}
}

#include "FileSystem/File.h"
#include "FileSystem/FileSystem.h"
#include "FileSystem/DynamicMemoryFile.h"
#include "FileSystem/KeyedArchive.h"
#include "Logger/Logger.h"
#include "Render/TextureDescriptor.h"
#include "Render/GPUFamilyDescriptor.h"
#include "Render/Image/ImageSystem.h"
#include "Render/Image/LibPVRHelper.h"
#include "Render/Image/LibDdsHelper.h"
#include "Utils/Utils.h"
#include "Utils/CRC32.h"
#include "Utils/StringFormat.h"

namespace DAVA
{
namespace Validator
{
DAVA_DEPRECATED(void CopyDX11CompressionFormat(TextureDescriptor& descriptor))
{
    if (!descriptor.IsCompressedFile())
    {
        if (descriptor.compression[GPU_DX11].format == FORMAT_INVALID && descriptor.compression[GPU_TEGRA].format != FORMAT_INVALID)
        {
            descriptor.compression[GPU_DX11] = descriptor.compression[GPU_TEGRA];
            if (descriptor.compression[GPU_DX11].format == FORMAT_ETC1)
            {
                descriptor.compression[GPU_DX11].format = FORMAT_DXT1;
            }
        }
    }
}

DAVA_DEPRECATED(void FixDX11CompressionFormat(TextureDescriptor& descriptor))
{
    if (!descriptor.IsCompressedFile())
    {
        if (descriptor.compression[GPU_DX11].format == FORMAT_RGB888)
        {
            descriptor.compression[GPU_DX11].Clear();
            Logger::Error("[TextureDescriptor] Texture %s has unsupported RGB888 format for GPU_DX11", descriptor.pathname.GetStringValue().c_str());
        }
    }
}

DAVA_DEPRECATED(void ConvertV10orLessToV11(TextureDescriptor& descriptor))
{
    descriptor.drawSettings.wrapModeS = (descriptor.drawSettings.wrapModeS == 0) ? rhi::TEXADDR_CLAMP : rhi::TEXADDR_WRAP;
    descriptor.drawSettings.wrapModeT = (descriptor.drawSettings.wrapModeT == 0) ? rhi::TEXADDR_CLAMP : rhi::TEXADDR_WRAP;

    switch (descriptor.drawSettings.minFilter)
    {
    case 0:
        descriptor.drawSettings.minFilter = rhi::TEXFILTER_NEAREST;
        descriptor.drawSettings.mipFilter = rhi::TEXMIPFILTER_NONE;
        break; //FILTER_NEAREST
    case 1:
        descriptor.drawSettings.minFilter = rhi::TEXFILTER_LINEAR;
        descriptor.drawSettings.mipFilter = rhi::TEXMIPFILTER_NONE;
        break; //FILTER_LINEAR
    case 2:
        descriptor.drawSettings.minFilter = rhi::TEXFILTER_NEAREST;
        descriptor.drawSettings.mipFilter = rhi::TEXMIPFILTER_NEAREST;
        break; //FILTER_NEAREST_MIPMAP_NEAREST
    case 3:
        descriptor.drawSettings.minFilter = rhi::TEXFILTER_LINEAR;
        descriptor.drawSettings.mipFilter = rhi::TEXMIPFILTER_NEAREST;
        break; //FILTER_LINEAR_MIPMAP_NEAREST
    case 4:
        descriptor.drawSettings.minFilter = rhi::TEXFILTER_NEAREST;
        descriptor.drawSettings.mipFilter = rhi::TEXMIPFILTER_LINEAR;
        break; //FILTER_NEAREST_MIPMAP_LINEAR
    case 5:
        descriptor.drawSettings.minFilter = rhi::TEXFILTER_LINEAR;
        descriptor.drawSettings.mipFilter = rhi::TEXMIPFILTER_LINEAR;
        break; //FILTER_LINEAR_MIPMAP_LINEAR
    default:
        break;
    }
}

DAVA_DEPRECATED(void FillImageContainerFromFormat(TextureDescriptor& descriptor))
{
    if (descriptor.IsCompressedFile())
    {
        if (GPUFamilyDescriptor::IsGPUForDevice(descriptor.gpu))
        {
            descriptor.imageFormat = GPUFamilyDescriptor::GetCompressedFileFormat(descriptor.gpu, descriptor.format);
        }
        else
        {
            descriptor.imageFormat = descriptor.dataSettings.sourceFileFormat;
        }
    }
    else
    {
        for (uint8 gpu = 0; gpu < eGPUFamily::GPU_FAMILY_COUNT; ++gpu)
        {
            descriptor.compression[gpu].imageFormat = GPUFamilyDescriptor::GetCompressedFileFormat(static_cast<eGPUFamily>(gpu), static_cast<PixelFormat>(descriptor.compression[gpu].format));
        }

        descriptor.compression[eGPUFamily::GPU_ORIGIN].imageFormat = descriptor.dataSettings.sourceFileFormat;
        descriptor.imageFormat = descriptor.dataSettings.sourceFileFormat;
    }
}
}

namespace TextureDescriptorLocal
{
String GetPostfix(eGPUFamily gpuFamily, ImageFormat imageFormat)
{
    String postfix = GPUFamilyDescriptor::GetGPUPrefix(gpuFamily) + ImageSystem::GetDefaultExtension(imageFormat);
    return postfix;
}

FilePath CreateCompressedTexturePathname(const FilePath& pathname, eGPUFamily gpuFamily, ImageFormat imageFormat)
{
    String postfix = GetPostfix(gpuFamily, imageFormat);
    return FilePath::CreateWithNewExtension(pathname, postfix);
}
}

//================   TextureDrawSettings  ===================
void TextureDescriptor::TextureDrawSettings::SetDefaultValues()
{
    wrapModeS = rhi::TEXADDR_WRAP;
    wrapModeT = rhi::TEXADDR_WRAP;

    minFilter = rhi::TEXFILTER_LINEAR;
    magFilter = rhi::TEXFILTER_LINEAR;
    mipFilter = rhi::TEXMIPFILTER_LINEAR;
}

//================   TextureDataSettings  ===================
void TextureDescriptor::TextureDataSettings::SetDefaultValues()
{
    textureFlags = FLAG_DEFAULT;
    cubefaceFlags = 0;

    static ImageFormat defaultImageFormat = ImageFormat::IMAGE_FORMAT_PNG;

    sourceFileFormat = defaultImageFormat;
    sourceFileExtension = ImageSystem::GetExtensionsFor(defaultImageFormat)[0];
}

void TextureDescriptor::TextureDataSettings::SetGenerateMipmaps(bool generateMipmaps)
{
    EnableFlag(generateMipmaps, FLAG_GENERATE_MIPMAPS);
}

bool TextureDescriptor::TextureDataSettings::GetGenerateMipMaps() const
{
    return IsFlagEnabled(FLAG_GENERATE_MIPMAPS);
}

void TextureDescriptor::TextureDataSettings::SetIsNormalMap(bool isNormalMap)
{
    EnableFlag(isNormalMap, FLAG_IS_NORMAL_MAP);
}

bool TextureDescriptor::TextureDataSettings::GetIsNormalMap() const
{
    return IsFlagEnabled(FLAG_IS_NORMAL_MAP);
}

void TextureDescriptor::TextureDataSettings::SetSeparateHDTextures(bool separateHDTextures)
{
    EnableFlag(separateHDTextures, FLAG_HAS_SEPARATE_HD_FILE);
}

bool TextureDescriptor::TextureDataSettings::GetSeparateHDTextures() const
{
    return IsFlagEnabled(FLAG_HAS_SEPARATE_HD_FILE);
}

void TextureDescriptor::TextureDataSettings::EnableFlag(bool enable, int8 flag)
{
    if (enable)
    {
        textureFlags |= flag;
    }
    else
    {
        textureFlags &= ~flag;
    }
}

bool TextureDescriptor::TextureDataSettings::IsFlagEnabled(int8 flag) const
{
    return ((textureFlags & flag) == flag);
}

//================   Compression  ===================
void TextureDescriptor::Compression::Clear()
{
    format = PixelFormat::FORMAT_INVALID;
    sourceFileCrc = 0;
    convertedFileCrc = 0;

    compressToWidth = 0;
    compressToHeight = 0;
}

//================   TextureDescriptor  ===================
const String TextureDescriptor::DESCRIPTOR_EXTENSION = ".tex";
const String TextureDescriptor::DEFAULT_CUBEFACE_EXTENSION = ".png";

TextureDescriptor::TextureDescriptor()
{
    SetDefaultValues();
}

TextureDescriptor* TextureDescriptor::CreateFromFile(const FilePath& filePathname)
{
    if (filePathname.IsEmpty() || (filePathname.GetType() == FilePath::PATH_IN_MEMORY))
        return nullptr;

    TextureDescriptor* descriptor = new TextureDescriptor();
    if (!descriptor->Initialize(filePathname))
    {
        Logger::Error("[TextureDescriptor::CreateFromFile(]: there are no descriptor file (%s).", filePathname.GetAbsolutePathname().c_str());
        delete descriptor;
        return nullptr;
    }

    return descriptor;
}

TextureDescriptor* TextureDescriptor::CreateDescriptor(rhi::TextureAddrMode wrap, bool generateMipmaps)
{
    TextureDescriptor* descriptor = new TextureDescriptor();
    descriptor->Initialize(wrap, generateMipmaps);

    return descriptor;
}

void TextureDescriptor::SetDefaultValues()
{
    pathname = FilePath();

    drawSettings.SetDefaultValues();
    dataSettings.SetDefaultValues();
    for (int32 i = 0; i < GPU_FAMILY_COUNT; ++i)
    {
        compression[i].Clear();
    }

    compression[eGPUFamily::GPU_ORIGIN].imageFormat = dataSettings.sourceFileFormat;
    imageFormat = dataSettings.sourceFileFormat;

    gpu = eGPUFamily::GPU_INVALID;
    format = PixelFormat::FORMAT_INVALID;

    isCompressedFile = false;
}

void TextureDescriptor::SetQualityGroup(const FastName& group)
{
    qualityGroup = group;
}

const FastName& TextureDescriptor::GetQualityGroup() const
{
    return qualityGroup;
}

bool TextureDescriptor::IsCompressedTextureActual(eGPUFamily forGPU) const
{
    const Compression* compressionForGPU = GetCompressionParams(forGPU);
    const uint32 sourceCRC = ReadSourceCRC();
    const uint32 convertedCRC = GetConvertedCRC(forGPU);

    const bool crcIsEqual = ((compressionForGPU->sourceFileCrc == sourceCRC) && (compressionForGPU->convertedFileCrc == convertedCRC)) && (convertedCRC != 0);
    if (crcIsEqual)
    {
        //this code need until using of convertation params in crc
        const ImageFormat imageFormat = GetImageFormatForGPU(forGPU);
        if (imageFormat == ImageFormat::IMAGE_FORMAT_UNKNOWN)
        {
            return false;
        }

        const FilePath filePath = TextureDescriptorLocal::CreateCompressedTexturePathname(pathname, forGPU, imageFormat);
        ImageInfo imageInfo = ImageSystem::GetImageInfo(filePath);

        const bool imageIsActual = (imageInfo.format == compressionForGPU->format) &&
        ((compressionForGPU->compressToWidth == 0) || (imageInfo.width == compressionForGPU->compressToWidth)) && ((compressionForGPU->compressToHeight == 0) || (imageInfo.height == compressionForGPU->compressToHeight));
        return imageIsActual;
    }

    return crcIsEqual;
}

bool TextureDescriptor::HasCompressionFor(eGPUFamily forGPU) const
{
    const Compression* compression = GetCompressionParams(forGPU);
    return (compression && compression->format != PixelFormat::FORMAT_INVALID && compression->imageFormat != ImageFormat::IMAGE_FORMAT_UNKNOWN);
}

bool TextureDescriptor::UpdateCrcForFormat(eGPUFamily forGPU) const
{
    bool wasUpdated = false;
    const Compression* compression = GetCompressionParams(forGPU);

    uint32 sourceCRC = ReadSourceCRC();
    if (compression->sourceFileCrc != sourceCRC)
    {
        compression->sourceFileCrc = sourceCRC;
        wasUpdated = true;
    }

    uint32 convertedCRC = GetConvertedCRC(forGPU);
    if (compression->convertedFileCrc != convertedCRC)
    {
        compression->convertedFileCrc = convertedCRC;
        wasUpdated = true;
    }

    return wasUpdated;
}

bool TextureDescriptor::Load(const FilePath& filePathname)
{
    ScopedPtr<File> file(File::Create(filePathname, File::READ | File::OPEN));
    if (!file)
    {
        Logger::Error("[TextureDescriptor::Load] Can't open file: %s", filePathname.GetAbsolutePathname().c_str());
        return false;
    }

    pathname = filePathname;

    int32 signature;
    file->Read(&signature);

    int8 version = 0;
    file->Read(&version);

    if (signature == COMPRESSED_FILE)
    {
        isCompressedFile = true;
    }
    else if (signature == NOTCOMPRESSED_FILE)
    {
        isCompressedFile = false;
    }
    else
    {
        Logger::Error("[TextureDescriptor::Load] Signature '%X' is incorrect", signature);
        return false;
    }

    switch (version)
    {
    case 6:
        LoadVersion6(file);
        break;
    case 7:
        LoadVersion7(file);
        break;
    case 8:
        LoadVersion8(file);
        break;
    case 9:
        LoadVersion9(file);
        break;
    case 10:
        LoadVersion10(file);
        break;
    case 11:
        LoadVersion11(file);
        break;
    case 12:
        LoadVersion12(file);
        break;
    case 13:
        LoadVersion12(file); // same as 12, but with HDR format support, descriptor structure has not been changed
        break;
    default:
    {
        Logger::Error("[TextureDescriptor::Load] Version %d is not supported", version);
        return false;
    }
    }

    { //validation & correction
        if (version < 10)
        { //10 is version of adding of DX11 GPU
            Validator::CopyDX11CompressionFormat(*this);
        }
        Validator::FixDX11CompressionFormat(*this);
        if (version < 12)
        {
            Validator::FillImageContainerFromFormat(*this);
        }
    }

    return true;
}

void TextureDescriptor::Save() const
{
    DVASSERT(!pathname.IsEmpty(), "Can use this method only after calling Load()");
    Save(pathname);
}

void TextureDescriptor::Save(const FilePath& filePathname) const
{
    ScopedPtr<File> file(File::Create(filePathname, File::WRITE | File::CREATE));
    if (!file)
    {
        Logger::Error("[TextureDescriptor::Save] Can't open file: %s", filePathname.GetAbsolutePathname().c_str());
        return;
    }

    SaveInternal(file, NOTCOMPRESSED_FILE, GPU_FAMILY_COUNT);
}

void TextureDescriptor::Export(const FilePath& filePathname, eGPUFamily forGPU) const
{
    ScopedPtr<File> file(File::Create(filePathname, File::WRITE | File::OPEN | File::CREATE));
    if (!file)
    {
        Logger::Error("[TextureDescriptor::Export] Can't open file: %s", filePathname.GetAbsolutePathname().c_str());
        return;
    }

    SaveInternal(file, COMPRESSED_FILE, forGPU);
}

namespace DescriptorFileOperation
{
void WriteCompression12(File* file, const TextureDescriptor::Compression& compression)
{
    uint8 imageFormat = static_cast<uint8>(compression.imageFormat);
    uint8 format = static_cast<uint8>(compression.format);
    file->Write(&imageFormat);
    file->Write(&format);
    file->Write(&compression.compressToWidth);
    file->Write(&compression.compressToHeight);
    file->Write(&compression.sourceFileCrc);
    file->Write(&compression.convertedFileCrc);
}
void ReadCompression12(File* file, TextureDescriptor::Compression& compression)
{
    uint8 imageFormat = IMAGE_FORMAT_UNKNOWN;
    uint8 format = FORMAT_INVALID;
    file->Read(&imageFormat);
    file->Read(&format);
    file->Read(&compression.compressToWidth);
    file->Read(&compression.compressToHeight);
    file->Read(&compression.sourceFileCrc);
    file->Read(&compression.convertedFileCrc);
    compression.imageFormat = static_cast<uint32>(imageFormat);
    compression.format = static_cast<int32>(format);
}

String ReadStringDeprecated(File* file)
{
    uint32 length = 0;
    Array<char8, 20> extStr;

    file->Read(&length);
    file->Read(extStr.data(), length);
    return String(extStr.data(), length);
}
}

void TextureDescriptor::OverridePathName(const FilePath& filename)
{
    pathname = filename;
    dataSettings.sourceFileExtension = filename.GetExtension();
}

void TextureDescriptor::SaveInternal(File* file, const int32 signature, const eGPUFamily forGPU) const
{
    file->Write(&signature);

    int8 version = CURRENT_VERSION;
    file->Write(&version);

    //draw settings
    file->Write(&drawSettings.wrapModeS);
    file->Write(&drawSettings.wrapModeT);
    file->Write(&drawSettings.minFilter);
    file->Write(&drawSettings.magFilter);
    file->Write(&drawSettings.mipFilter);

    //data settings
    file->Write(&dataSettings.textureFlags);
    file->Write(&dataSettings.cubefaceFlags);
    file->Write(&dataSettings.sourceFileFormat);

    file->WriteString(dataSettings.sourceFileExtension);
    for (int i = 0; i < Texture::CUBE_FACE_COUNT; ++i)
    {
        if (dataSettings.cubefaceFlags & (1 << i))
        {
            file->WriteString(GetFaceExtension(i));
        }
    }

    //compressions
    if (forGPU == GPU_FAMILY_COUNT)
    { // save internal data for compression
        uint8 compressionCount = GPU_FAMILY_COUNT;
        file->Write(&compressionCount);
        for (uint8 i = 0; i < compressionCount; ++i)
        {
            DescriptorFileOperation::WriteCompression12(file, compression[i]);
        }
    }
    else if (forGPU != GPU_INVALID)
    { //export
        uint8 compressionCount = 0;
        file->Write(&compressionCount);

        uint8 exportedAsGpu = forGPU;
        uint8 exportedAsContainer = compression[forGPU].imageFormat;
        uint8 exportedAsFormat = compression[forGPU].format;
        file->Write(&exportedAsGpu);
        file->Write(&exportedAsContainer);
        file->Write(&exportedAsFormat);
    }
    else
    {
        DVASSERT(false, Format("Saving for wrong gpu %d was selected", forGPU).c_str());
    }
}

void TextureDescriptor::LoadVersion6(DAVA::File* file)
{
    file->Read(&drawSettings.wrapModeS);
    file->Read(&drawSettings.wrapModeT);
    file->Read(&dataSettings.textureFlags);
    file->Read(&drawSettings.minFilter);
    file->Read(&drawSettings.magFilter);

    Validator::ConvertV10orLessToV11(*this);

    if (isCompressedFile)
    {
        int8 exportedAsGpuFamily = 0;
        file->Read(&exportedAsGpuFamily);
        gpu = GPUFamilyDescriptor::ConvertValueToGPU(exportedAsGpuFamily);

        int8 exportedAsPixelFormat = FORMAT_INVALID;
        file->Read(&exportedAsPixelFormat);
        format = static_cast<PixelFormat>(exportedAsPixelFormat);
    }
    else
    {
        for (auto i = 0; i < 5; ++i)
        {
            int8 format;
            file->Read(&format);
            compression[i].format = static_cast<PixelFormat>(format);

            file->Read(&compression[i].compressToWidth);
            file->Read(&compression[i].compressToHeight);
            file->Read(&compression[i].sourceFileCrc);
        }

        RecalculateCompressionSourceCRC();
    }
}

void TextureDescriptor::LoadVersion7(DAVA::File* file)
{
    file->Read(&drawSettings.wrapModeS);
    file->Read(&drawSettings.wrapModeT);
    file->Read(&dataSettings.textureFlags);
    file->Read(&drawSettings.minFilter);
    file->Read(&drawSettings.magFilter);

    Validator::ConvertV10orLessToV11(*this);

    if (isCompressedFile)
    {
        int8 exportedAsGpuFamily = 0;
        file->Read(&exportedAsGpuFamily);
        gpu = GPUFamilyDescriptor::ConvertValueToGPU(exportedAsGpuFamily);

        int8 exportedAsPixelFormat = FORMAT_INVALID;
        file->Read(&exportedAsPixelFormat);
        format = static_cast<PixelFormat>(exportedAsPixelFormat);
    }
    else
    {
        for (int32 i = 0; i < 5; ++i)
        {
            int8 format;
            file->Read(&format);
            compression[i].format = static_cast<PixelFormat>(format);

            file->Read(&compression[i].compressToWidth);
            file->Read(&compression[i].compressToHeight);
            file->Read(&compression[i].sourceFileCrc);
            file->Read(&compression[i].convertedFileCrc);
        }

        RecalculateCompressionSourceCRC();
    }

    file->Read(&dataSettings.cubefaceFlags);
}

void TextureDescriptor::LoadVersion8(File* file)
{
    file->Read(&drawSettings.wrapModeS);
    file->Read(&drawSettings.wrapModeT);
    file->Read(&dataSettings.textureFlags);
    file->Read(&drawSettings.minFilter);
    file->Read(&drawSettings.magFilter);

    Validator::ConvertV10orLessToV11(*this);

    if (isCompressedFile)
    {
        int8 exportedAsGpuFamily = 0;
        file->Read(&exportedAsGpuFamily);
        gpu = GPUFamilyDescriptor::ConvertValueToGPU(exportedAsGpuFamily);

        uint8 exportedAsPixelFormat = FORMAT_INVALID;
        file->Read(&exportedAsPixelFormat);
        format = static_cast<PixelFormat>(exportedAsPixelFormat);
    }
    else
    {
        uint8 compressionsCount = 0;
        file->Read(&compressionsCount);
        for (auto i = 0; i < 6; ++i)
        {
            uint8 format;
            file->Read(&format);
            compression[i].format = PixelFormat(format);

            file->Read(&compression[i].compressToWidth);
            file->Read(&compression[i].compressToHeight);
            file->Read(&compression[i].sourceFileCrc);
            file->Read(&compression[i].convertedFileCrc);
        }

        RecalculateCompressionSourceCRC();
    }

    file->Read(&dataSettings.cubefaceFlags);
}

void TextureDescriptor::RecalculateCompressionSourceCRC()
{
    auto sourceCrcOld = ReadSourceCRC_V8_or_less();
    auto sourceCrcNew = ReadSourceCRC();

    for (Compression& compr : compression)
    {
        if (compr.sourceFileCrc == sourceCrcOld)
        {
            compr.sourceFileCrc = sourceCrcNew;
        }
    }
}

uint32 TextureDescriptor::ReadSourceCRC_V8_or_less() const
{
    uint32 crc = 0;

    DAVA::File* f = DAVA::File::Create(GetSourceTexturePathname(), DAVA::File::OPEN | DAVA::File::READ);
    if (f != nullptr)
    {
        uint8 buffer[8];

        // Read PNG header
        f->Read(buffer, 8);

        // read chunk header
        while (0 != f->Read(buffer, 8))
        {
            int32 chunk_size = 0;
            chunk_size |= (buffer[0] << 24);
            chunk_size |= (buffer[1] << 16);
            chunk_size |= (buffer[2] << 8);
            chunk_size |= buffer[3];

            // jump thought data
            DVASSERT(chunk_size >= 0);
            f->Seek(chunk_size, File::SEEK_FROM_CURRENT);

            // read crc
            f->Read(buffer, 4);
            crc += (reinterpret_cast<uint32*>(buffer))[0];
        }

        f->Release();
    }

    return crc;
}

void TextureDescriptor::LoadVersion9(File* file)
{
    //draw settings
    file->Read(&drawSettings.wrapModeS);
    file->Read(&drawSettings.wrapModeT);
    file->Read(&drawSettings.minFilter);
    file->Read(&drawSettings.magFilter);

    Validator::ConvertV10orLessToV11(*this);

    //data settings
    file->Read(&dataSettings.textureFlags);
    file->Read(&dataSettings.cubefaceFlags);

    int8 sourceFileFormat = 0;
    file->Read(&sourceFileFormat);
    dataSettings.sourceFileFormat = static_cast<ImageFormat>(sourceFileFormat);
    dataSettings.sourceFileExtension = DescriptorFileOperation::ReadStringDeprecated(file);
    for (int i = 0; i < Texture::CUBE_FACE_COUNT; ++i)
    {
        if (dataSettings.cubefaceFlags & (1 << i))
        {
            dataSettings.cubefaceExtensions[i] = DescriptorFileOperation::ReadStringDeprecated(file);
        }
    }

    //compression
    uint8 compressionsCount = 0;
    file->Read(&compressionsCount);
    for (uint8 i = 0; i < compressionsCount; ++i)
    {
        Compression& nextCompression = compression[i];

        uint8 formatLocal = PixelFormat::FORMAT_INVALID;
        file->Read(&formatLocal);
        nextCompression.format = static_cast<PixelFormat>(formatLocal);

        file->Read(&nextCompression.compressToWidth);
        file->Read(&nextCompression.compressToHeight);
        file->Read(&nextCompression.sourceFileCrc);
        file->Read(&nextCompression.convertedFileCrc);
    }

    //export data
    int8 exportedAsGpuFamily = GPU_INVALID;
    file->Read(&exportedAsGpuFamily);
    gpu = GPUFamilyDescriptor::ConvertValueToGPU(exportedAsGpuFamily);

    uint8 exportedAsPixelFormat = FORMAT_INVALID;
    file->Read(&exportedAsPixelFormat);
    format = static_cast<PixelFormat>(exportedAsPixelFormat);
}

void TextureDescriptor::LoadVersion10(File* file)
{
    // has no changes in format
    LoadVersion9(file);
}

void TextureDescriptor::LoadVersion11(File* file)
{
    //draw settings
    file->Read(&drawSettings.wrapModeS);
    file->Read(&drawSettings.wrapModeT);
    file->Read(&drawSettings.minFilter);
    file->Read(&drawSettings.magFilter);
    file->Read(&drawSettings.mipFilter);

    //data settings
    file->Read(&dataSettings.textureFlags);
    file->Read(&dataSettings.cubefaceFlags);

    int8 sourceFileFormat = 0;
    file->Read(&sourceFileFormat);
    dataSettings.sourceFileFormat = static_cast<ImageFormat>(sourceFileFormat);
    dataSettings.sourceFileExtension = DescriptorFileOperation::ReadStringDeprecated(file);

    for (int i = 0; i < Texture::CUBE_FACE_COUNT; ++i)
    {
        if (dataSettings.cubefaceFlags & (1 << i))
        {
            dataSettings.cubefaceExtensions[i] = DescriptorFileOperation::ReadStringDeprecated(file);
        }
    }

    //compression
    uint8 compressionsCount = 0;
    file->Read(&compressionsCount);
    for (uint8 i = 0; i < compressionsCount; ++i)
    {
        Compression& nextCompression = compression[i];

        int8 formatLocal = PixelFormat::FORMAT_INVALID;
        file->Read(&formatLocal);
        nextCompression.format = static_cast<PixelFormat>(formatLocal);

        file->Read(&nextCompression.compressToWidth);
        file->Read(&nextCompression.compressToHeight);
        file->Read(&nextCompression.sourceFileCrc);
        file->Read(&nextCompression.convertedFileCrc);
    }

    //export data
    int8 exportedAsGpuFamily = GPU_INVALID;
    file->Read(&exportedAsGpuFamily);
    gpu = GPUFamilyDescriptor::ConvertValueToGPU(exportedAsGpuFamily);

    uint8 exportedAsPixelFormat = FORMAT_INVALID;
    file->Read(&exportedAsPixelFormat);
    format = static_cast<PixelFormat>(exportedAsPixelFormat);
}

void TextureDescriptor::LoadVersion12(File* file)
{
    //draw settings
    file->Read(&drawSettings.wrapModeS);
    file->Read(&drawSettings.wrapModeT);
    file->Read(&drawSettings.minFilter);
    file->Read(&drawSettings.magFilter);
    file->Read(&drawSettings.mipFilter);

    //data settings
    file->Read(&dataSettings.textureFlags);
    file->Read(&dataSettings.cubefaceFlags);

    int8 sourceFileFormat = 0;
    file->Read(&sourceFileFormat);
    dataSettings.sourceFileFormat = static_cast<ImageFormat>(sourceFileFormat);
    dataSettings.sourceFileExtension.clear();
    file->ReadString(dataSettings.sourceFileExtension);
    for (int i = 0; i < Texture::CUBE_FACE_COUNT; ++i)
    {
        if (dataSettings.cubefaceFlags & (1 << i))
        {
            dataSettings.cubefaceExtensions[i].clear();
            file->ReadString(dataSettings.cubefaceExtensions[i]);
        }
    }

    //compression
    uint8 compressionsCount = 0;
    file->Read(&compressionsCount);
    if (compressionsCount == eGPUFamily::GPU_FAMILY_COUNT)
    {
        for (uint8 i = 0; i < compressionsCount; ++i)
        {
            DescriptorFileOperation::ReadCompression12(file, compression[i]);
        }
    }
    else
    {
        //export data
        uint8 exportedAsGpu = eGPUFamily::GPU_INVALID;
        uint8 exportedAsContainer = ImageFormat::IMAGE_FORMAT_UNKNOWN;
        uint8 exportedAsFormat = PixelFormat::FORMAT_INVALID;
        file->Read(&exportedAsGpu);
        file->Read(&exportedAsContainer);
        file->Read(&exportedAsFormat);

        gpu = GPUFamilyDescriptor::ConvertValueToGPU(exportedAsGpu);
        imageFormat = static_cast<ImageFormat>(exportedAsContainer);
        format = static_cast<PixelFormat>(exportedAsFormat);
    }
}

bool TextureDescriptor::GetGenerateMipMaps() const
{
    return dataSettings.GetGenerateMipMaps();
}

FilePath TextureDescriptor::GetSourceTexturePathname() const
{
    if (pathname.IsEmpty())
    {
        return FilePath();
    }

    return FilePath::CreateWithNewExtension(pathname, dataSettings.sourceFileExtension);
}

const String& TextureDescriptor::GetFaceExtension(uint32 face) const
{
    return (
    dataSettings.cubefaceExtensions[face].empty() ?
    GetDefaultFaceExtension() :
    dataSettings.cubefaceExtensions[face]);
}

void TextureDescriptor::GetFacePathnames(Vector<FilePath>& faceNames) const
{
    GenerateFacePathnames(pathname, Texture::FACE_NAME_SUFFIX, faceNames);
}

void TextureDescriptor::GenerateFacePathnames(const FilePath& filePath, const Array<String, Texture::CUBE_FACE_COUNT>& faceNameSuffixes, Vector<FilePath>& faceNames) const
{
    faceNames.resize(Texture::CUBE_FACE_COUNT, FilePath());

    String baseName = filePath.GetBasename();
    for (uint32 face = 0; face < Texture::CUBE_FACE_COUNT; ++face)
    {
        if (dataSettings.cubefaceFlags & (1 << face))
        {
            faceNames[face] = filePath;
            faceNames[face].ReplaceFilename(baseName + faceNameSuffixes[face] + GetFaceExtension(face));
        }
    }
}

FilePath TextureDescriptor::GetDescriptorPathname(const FilePath& texturePathname)
{
    DVASSERT(!texturePathname.IsEmpty());

    if (0 == CompareCaseInsensitive(texturePathname.GetExtension(), GetDescriptorExtension()))
    {
        return texturePathname;
    }
    else
    {
        return FilePath::CreateWithNewExtension(texturePathname, GetDescriptorExtension());
    }
}

const String& TextureDescriptor::GetDescriptorExtension()
{
    return DESCRIPTOR_EXTENSION;
}

const String& TextureDescriptor::GetDefaultFaceExtension()
{
    return DEFAULT_CUBEFACE_EXTENSION;
}

const String& TextureDescriptor::GetLightmapTextureExtension()
{
    return ImageSystem::GetExtensionsFor(IMAGE_FORMAT_PNG)[0];
}

const TextureDescriptor::Compression* TextureDescriptor::GetCompressionParams(eGPUFamily gpuFamily) const
{
    DVASSERT(gpuFamily >= 0 && gpuFamily < GPU_FAMILY_COUNT);
    return &compression[gpuFamily];
}

Array<ImageFormat, 6> TextureDescriptor::sourceTextureTypes = { { IMAGE_FORMAT_PNG, IMAGE_FORMAT_TGA, IMAGE_FORMAT_JPEG, IMAGE_FORMAT_DDS, IMAGE_FORMAT_WEBP, IMAGE_FORMAT_HDR } };
Array<ImageFormat, 2> TextureDescriptor::compressedTextureTypes = { { IMAGE_FORMAT_PVR, IMAGE_FORMAT_DDS } };

bool IsFormatSupported(ImageFormat format, const String& extension)
{
    const Vector<String>& extensions = ImageSystem::GetExtensionsFor(format);
    for (const String& ext : extensions)
    {
        if (CompareCaseInsensitive(ext, extension) == 0)
            return true;
    }
    return false;
}

bool TextureDescriptor::IsSupportedSourceFormat(ImageFormat imageFormat)
{
    auto found = std::find(sourceTextureTypes.begin(), sourceTextureTypes.end(), imageFormat);
    return (found != sourceTextureTypes.end());
}

bool TextureDescriptor::IsSupportedCompressedFormat(ImageFormat imageFormat)
{
    auto found = std::find(compressedTextureTypes.begin(), compressedTextureTypes.end(), imageFormat);
    return (found != compressedTextureTypes.end());
}

bool TextureDescriptor::IsSourceTextureExtension(const String& extension)
{
    for (ImageFormat textureType : sourceTextureTypes)
    {
        if (IsFormatSupported(textureType, extension))
            return true;
    }

    return false;
}

bool TextureDescriptor::IsCompressedTextureExtension(const String& extension)
{
    for (ImageFormat textureType : compressedTextureTypes)
    {
        if (IsFormatSupported(textureType, extension))
            return true;
    }

    return false;
}

bool TextureDescriptor::IsDescriptorExtension(const String& extension)
{
    return (CompareCaseInsensitive(GetDescriptorExtension(), extension) == 0);
}

bool TextureDescriptor::IsSupportedTextureExtension(const String& extension)
{
    return (IsSourceTextureExtension(extension) ||
            IsCompressedTextureExtension(extension) ||
            IsDescriptorExtension(extension));
}

bool TextureDescriptor::IsCompressedFile() const
{
    return isCompressedFile;
}

bool TextureDescriptor::IsCubeMap() const
{
    return (dataSettings.cubefaceFlags != 0);
}

uint32 TextureDescriptor::ReadSourceCRC() const
{
    return CRC32::ForFile(GetSourceTexturePathname());
}

uint32 TextureDescriptor::GetConvertedCRC(eGPUFamily forGPU) const
{
    if (forGPU == eGPUFamily::GPU_ORIGIN || compression[forGPU].format == FORMAT_INVALID)
        return 0;

    ImageFormat imageFormat = GetImageFormatForGPU(forGPU);
    if (imageFormat == ImageFormat::IMAGE_FORMAT_UNKNOWN)
    {
        return 0;
    }

    FilePath filePath = TextureDescriptorLocal::CreateCompressedTexturePathname(pathname, forGPU, imageFormat);
    if (imageFormat == IMAGE_FORMAT_PVR)
    {
#ifdef __DAVAENGINE_WIN_UAP__
        Logger::Error("[TextureDescriptor::GetConvertedCRC] can't get compressed texture filename for %s; "
                      "LibPVR is unsupported",
                      filePath.GetStringValue().c_str());
        DVASSERT(false);
        return 0;
#else
        uint32 convertedCRC = LibPVRHelper::GetCRCFromMetaData(filePath);
        if (convertedCRC != 0)
        {
            convertedCRC += GenerateDescriptorCRC(forGPU);
        }
        return convertedCRC;
#endif
    }
    else if (imageFormat == IMAGE_FORMAT_DDS)
    {
        uint32 convertedCRC = LibDdsHelper::GetCRCFromMetaData(filePath);
        if (convertedCRC != 0)
        {
            convertedCRC += GenerateDescriptorCRC(forGPU);
        }
        return convertedCRC;
    }
    else
    {
        Logger::Error("[TextureDescriptor::GetConvertedCRC] can't get compressed texture filename for %s", filePath.GetStringValue().c_str());
        DVASSERT(false);
        return 0;
    }
}

FilePath TextureDescriptor::CreateMultiMipPathnameForGPU(const eGPUFamily gpuFamily) const
{
    if (GPUFamilyDescriptor::IsGPUForDevice(gpuFamily))
    {
        ImageFormat imageFormat = GetImageFormatForGPU(gpuFamily);
        if (imageFormat == ImageFormat::IMAGE_FORMAT_UNKNOWN)
            return FilePath();

        String postfix = TextureDescriptorLocal::GetPostfix(gpuFamily, imageFormat);
        return FilePath::CreateWithNewExtension(pathname, postfix);
    }

    return GetSourceTexturePathname();
}

bool TextureDescriptor::CreateSingleMipPathnamesForGPU(const eGPUFamily gpuFamily, Vector<FilePath>& pathes) const
{
    if ((dataSettings.textureFlags & TextureDataSettings::FLAG_HAS_SEPARATE_HD_FILE) && GPUFamilyDescriptor::IsGPUForDevice(gpuFamily))
    {
        ImageFormat imageFormat = GetImageFormatForGPU(gpuFamily);
        if (imageFormat == ImageFormat::IMAGE_FORMAT_UNKNOWN)
        {
            return false;
        }

        String postfix = TextureDescriptorLocal::GetPostfix(gpuFamily, imageFormat);
        pathes.emplace_back(FilePath::CreateWithNewExtension(pathname, ".hd" + postfix));

        return true;
    }

    return false;
}

void TextureDescriptor::CreateLoadPathnamesForGPU(const eGPUFamily gpuFamily, Vector<FilePath>& pathes) const
{
    CreateSingleMipPathnamesForGPU(gpuFamily, pathes);
    FilePath path = CreateMultiMipPathnameForGPU(gpuFamily);
    if (!path.IsEmpty())
    {
        pathes.emplace_back(path);
    }
}

PixelFormat TextureDescriptor::GetPixelFormatForGPU(eGPUFamily forGPU) const
{
    if (forGPU == eGPUFamily::GPU_INVALID)
        return PixelFormat::FORMAT_INVALID;

    DVASSERT(0 <= forGPU && forGPU < GPU_FAMILY_COUNT);
    return static_cast<PixelFormat>(compression[forGPU].format);
}

ImageFormat TextureDescriptor::GetImageFormatForGPU(eGPUFamily forGPU) const
{
    ImageFormat retValue = ImageFormat::IMAGE_FORMAT_UNKNOWN;
    if (eGPUFamily::GPU_INVALID == forGPU)
    {
        retValue = dataSettings.sourceFileFormat;
    }
    else if (IsCompressedFile())
    {
        retValue = imageFormat;
    }
    else
    {
        retValue = static_cast<ImageFormat>(compression[forGPU].imageFormat);
    }

    bool imageFormatIsValid = (ImageFormat::IMAGE_FORMAT_UNKNOWN == retValue) || ((ImageFormat::IMAGE_FORMAT_PNG <= retValue) && (retValue < ImageFormat::IMAGE_FORMAT_COUNT));
    if (imageFormatIsValid == false)
    {
        Logger::Error("[%s] Seems that file is corrupted, %s", __FUNCTION__, pathname.GetStringValue().c_str());
        return ImageFormat::IMAGE_FORMAT_UNKNOWN;
    }

    return retValue;
}

void TextureDescriptor::Initialize(rhi::TextureAddrMode wrap, bool generateMipmaps)
{
    SetDefaultValues();

    drawSettings.wrapModeS = wrap;
    drawSettings.wrapModeT = wrap;

    dataSettings.SetGenerateMipmaps(generateMipmaps);
    drawSettings.minFilter = rhi::TEXFILTER_LINEAR;
    drawSettings.magFilter = rhi::TEXFILTER_LINEAR;

    if (generateMipmaps)
    {
        drawSettings.mipFilter = rhi::TEXMIPFILTER_LINEAR;
    }
    else
    {
        drawSettings.mipFilter = rhi::TEXMIPFILTER_NONE;
    }
}

void TextureDescriptor::Initialize(const TextureDescriptor* descriptor)
{
    if (nullptr == descriptor)
    {
        SetDefaultValues();
        return;
    }

    pathname = descriptor->pathname;

    drawSettings = descriptor->drawSettings;
    dataSettings = descriptor->dataSettings;

    isCompressedFile = descriptor->isCompressedFile;
    for (uint32 i = 0; i < GPU_FAMILY_COUNT; ++i)
    {
        compression[i] = descriptor->compression[i];
    }

    qualityGroup = descriptor->qualityGroup;

    gpu = descriptor->gpu;
    imageFormat = descriptor->imageFormat;
    format = descriptor->format;
}

bool TextureDescriptor::Initialize(const FilePath& filePathname)
{
    SetDefaultValues();

    FilePath descriptorPathname = GetDescriptorPathname(filePathname);
    return Load(descriptorPathname);
}

void TextureDescriptor::SetGenerateMipmaps(bool generateMipmaps)
{
    dataSettings.SetGenerateMipmaps(generateMipmaps);
}

bool TextureDescriptor::Reload()
{
    if (FileSystem::Instance()->Exists(pathname))
    {
        FilePath descriptorPathname = pathname;
        SetDefaultValues();
        return Load(descriptorPathname);
    }

    return false;
}

uint32 TextureDescriptor::GenerateDescriptorCRC(eGPUFamily forGPU) const
{
    static const uint32 CRC_BUFFER_SIZE = 16;
    static const uint8 CRC_BUFFER_VALUE = 0x3F; //to create nonzero buffer

    Array<uint8, CRC_BUFFER_SIZE> crcBuffer; //this buffer need to calculate correct CRC of texture descriptor. I plan to fill this buffer with params that are important for compression of textures sush as textureFlags
    Memset(crcBuffer.data(), CRC_BUFFER_VALUE, crcBuffer.size());

    uint8* bufferPtr = crcBuffer.data();
    *bufferPtr++ = dataSettings.textureFlags;

#if 0   
    // We need to enable this code before global re-saving or reloading of texture descriptors. 
    // This code will affect "Not Relevant" image in texture descriptors and we need custom loading of old descripters

    // add convertation params
    *bufferPtr++ = compression[forGPU].format;

    const uint32 sizeOfWidth = sizeof(compression[forGPU].compressToWidth);
    Memcpy(bufferPtr, &compression[forGPU].compressToWidth, sizeOfWidth);
    bufferPtr += sizeOfWidth;

    const uint32 sizeOfHeight = sizeof(compression[forGPU].compressToHeight);
    Memcpy(bufferPtr, &compression[forGPU].compressToHeight, sizeOfHeight);
    bufferPtr += sizeOfHeight;
    //end of convertation params
#endif //

    return CRC32::ForBuffer(reinterpret_cast<const char8*>(crcBuffer.data()), CRC_BUFFER_SIZE);
}

bool TextureDescriptor::IsPresetValid(const KeyedArchive* presetArchive) const
{
    DVASSERT(IsCompressedFile() == false);
    DVASSERT(presetArchive);

    const String object = presetArchive->GetString("object");
    if (object != "TextureDescriptor")
    {
        Logger::Error("Cannot load %s archive as TextureDescriptor preset", object.c_str());
        return false;
    }

    const int32 version = presetArchive->GetInt32("version");
    if (version < CURRENT_VERSION)
    {
        Logger::Warning("Loading old preset version %d. Current is %d", version, CURRENT_VERSION);
    }
    else if (version > CURRENT_VERSION)
    {
        Logger::Error("Trying to load newer version %d than have %d", version, CURRENT_VERSION);
        return false;
    }

    const uint8 cubefaceFlags = presetArchive->GetInt32("cubefaceFlags");
    if (cubefaceFlags != dataSettings.cubefaceFlags)
    {
        Logger::Error("Cannot apply preset with different cubefaceFlags");
        return false;
    }

    return true;
}

bool TextureDescriptor::DeserializeFromPreset(const KeyedArchive* presetArchive)
{
    if (!IsPresetValid(presetArchive))
        return false;

    drawSettings.wrapModeS = static_cast<int8>(presetArchive->GetInt32("wrapModeS", rhi::TEXADDR_WRAP));
    drawSettings.wrapModeT = static_cast<int8>(presetArchive->GetInt32("wrapModeT", rhi::TEXADDR_WRAP));
    drawSettings.minFilter = static_cast<int8>(presetArchive->GetInt32("minFilter", rhi::TEXFILTER_LINEAR));
    drawSettings.magFilter = static_cast<int8>(presetArchive->GetInt32("magFilter", rhi::TEXFILTER_LINEAR));
    drawSettings.mipFilter = static_cast<int8>(presetArchive->GetInt32("mipFilter", rhi::TEXMIPFILTER_LINEAR));

    dataSettings.textureFlags = static_cast<int8>(presetArchive->GetInt32("textureFlags", TextureDescriptor::TextureDataSettings::FLAG_DEFAULT));

    for (uint32 forGPU = 0; forGPU < GPU_FAMILY_COUNT; ++forGPU)
    {
        String gpuName = GPUFamilyDescriptor::GetGPUName(static_cast<eGPUFamily>(forGPU));
        const KeyedArchive* compressionArchive = presetArchive->GetArchive(gpuName);

        TextureDescriptor::Compression& compressionGPU = compression[forGPU];
        if (compressionArchive != nullptr)
        {
            int32 formatAsInt = compressionArchive->GetInt32("format", FORMAT_INVALID);
            int32 imageFormatAsInt = compressionArchive->GetInt32("imageFormat", IMAGE_FORMAT_UNKNOWN);
            int32 compressToWidth = compressionArchive->GetInt32("width");
            int32 compressToHeight = compressionArchive->GetInt32("height");

            if (formatAsInt != compressionGPU.format
                || compressToWidth != compressionGPU.compressToWidth
                || compressToHeight != compressionGPU.compressToHeight
                || imageFormatAsInt != compressionGPU.imageFormat
                )
            {
                compressionGPU.imageFormat = imageFormatAsInt;
                compressionGPU.format = formatAsInt;
                compressionGPU.compressToHeight = compressToHeight;
                compressionGPU.compressToWidth = compressToWidth;
                compressionGPU.convertedFileCrc = 0;
            }
        }
    }

    if (presetArchive->GetInt32("version") < 12)
    {
        Validator::FillImageContainerFromFormat(*this);
    }

    return true;
}

bool TextureDescriptor::SerializeToPreset(KeyedArchive* presetArchive) const
{
    DVASSERT(IsCompressedFile() == false);

    presetArchive->SetString("object", "TextureDescriptor");
    presetArchive->SetInt32("version", CURRENT_VERSION);

    presetArchive->SetInt32("wrapModeS", drawSettings.wrapModeS);
    presetArchive->SetInt32("wrapModeT", drawSettings.wrapModeT);
    presetArchive->SetInt32("minFilter", drawSettings.minFilter);
    presetArchive->SetInt32("magFilter", drawSettings.magFilter);
    presetArchive->SetInt32("mipFilter", drawSettings.mipFilter);

    presetArchive->SetInt32("textureFlags", dataSettings.textureFlags);
    presetArchive->SetInt32("cubefaceFlags", dataSettings.cubefaceFlags);

    for (uint8 forGPU = 0; forGPU < GPU_FAMILY_COUNT; ++forGPU)
    {
        int32 width = compression[forGPU].compressToWidth;
        int32 height = compression[forGPU].compressToHeight;

        if (compression[forGPU].format == FORMAT_INVALID && width > 0 && height > 0)
        {
            return false;
        }

        ScopedPtr<KeyedArchive> compressionArchive(new KeyedArchive());
        compressionArchive->SetInt32("format", compression[forGPU].format);
        compressionArchive->SetInt32("imageFormat", compression[forGPU].imageFormat);
        compressionArchive->SetInt32("width", width);
        compressionArchive->SetInt32("height", height);

        String gpuName = GPUFamilyDescriptor::GetGPUName(static_cast<eGPUFamily>(forGPU));
        presetArchive->SetArchive(gpuName, compressionArchive);
    }

    return true;
}
};

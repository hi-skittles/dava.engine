#pragma once

#include "Base/BaseTypes.h"
#include "Base/BaseObject.h"
#include "FileSystem/FilePath.h"
#include "Render/Texture.h"
#include "Utils/MD5.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Info: all helper code should be moved into Render/TextureDescriptorUtils.h
////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace DAVA
{
class File;
class TextureDescriptor final
{
    static const String DESCRIPTOR_EXTENSION;
    static const String DEFAULT_CUBEFACE_EXTENSION;

    enum eSignatures : uint32
    {
        COMPRESSED_FILE = 0x00EEEE00,
        NOTCOMPRESSED_FILE = 0x00EE00EE
    };

public:
    static const int8 CURRENT_VERSION = 13;

    struct TextureDrawSettings : public InspBase
    {
        void SetDefaultValues();

        int8 wrapModeS = rhi::TEXADDR_WRAP;
        int8 wrapModeT = rhi::TEXADDR_WRAP;

        int8 minFilter = rhi::TEXFILTER_LINEAR;
        int8 magFilter = rhi::TEXFILTER_LINEAR;
        int8 mipFilter = rhi::TEXMIPFILTER_LINEAR;

        INTROSPECTION(TextureDrawSettings,
                      MEMBER(wrapModeS, InspDesc("wrapModeS", GlobalEnumMap<rhi::TextureAddrMode>::Instance()), I_VIEW | I_EDIT | I_SAVE)
                      MEMBER(wrapModeT, InspDesc("wrapModeT", GlobalEnumMap<rhi::TextureAddrMode>::Instance()), I_VIEW | I_EDIT | I_SAVE)
                      MEMBER(minFilter, InspDesc("minFilter", GlobalEnumMap<rhi::TextureFilter>::Instance()), I_VIEW | I_EDIT | I_SAVE)
                      MEMBER(magFilter, InspDesc("magFilter", GlobalEnumMap<rhi::TextureFilter>::Instance()), I_VIEW | I_EDIT | I_SAVE)
                      MEMBER(mipFilter, InspDesc("mipFilter", GlobalEnumMap<rhi::TextureMipFilter>::Instance()), I_VIEW | I_EDIT | I_SAVE))
    };

    struct TextureDataSettings : public InspBase
    {
        enum eOptionsFlag : uint8
        {
            FLAG_GENERATE_MIPMAPS = 1 << 0,
            FLAG_IS_NORMAL_MAP = 1 << 1,
            FLAG_HAS_SEPARATE_HD_FILE = 1 << 2,

            FLAG_INVALID = 1 << 7,

            FLAG_DEFAULT = FLAG_GENERATE_MIPMAPS
        };

        TextureDataSettings()
        {
            SetDefaultValues();
        }
        void SetDefaultValues();

        void SetGenerateMipmaps(bool generateMipmaps);
        bool GetGenerateMipMaps() const;

        void SetIsNormalMap(bool isNormalMap);
        bool GetIsNormalMap() const;

        void SetSeparateHDTextures(bool separateHDTextures);
        bool GetSeparateHDTextures() const;

        String cubefaceExtensions[Texture::CUBE_FACE_COUNT];
        String sourceFileExtension;
        uint8 textureFlags = eOptionsFlag::FLAG_DEFAULT;
        uint8 cubefaceFlags = 0;
        ImageFormat sourceFileFormat = ImageFormat::IMAGE_FORMAT_UNKNOWN;

        INTROSPECTION(TextureDataSettings,
                      PROPERTY("generateMipMaps", "generateMipMaps", GetGenerateMipMaps, SetGenerateMipmaps, I_VIEW | I_EDIT | I_SAVE)
                      PROPERTY("isNormalMap", "isNormalMap", GetIsNormalMap, SetIsNormalMap, I_VIEW | I_EDIT | I_SAVE)
                      MEMBER(cubefaceFlags, "cubefaceFlags", I_SAVE)
                      MEMBER(sourceFileFormat, "sourceFileFormat", I_SAVE)
                      MEMBER(sourceFileExtension, "sourceFileExtension", I_SAVE)
                      )

    private:
        void EnableFlag(bool enable, int8 flag);
        bool IsFlagEnabled(int8 flag) const;
    };

    struct Compression : public InspBase
    {
        int32 format = PixelFormat::FORMAT_INVALID;
        uint32 imageFormat = ImageFormat::IMAGE_FORMAT_UNKNOWN;
        mutable uint32 sourceFileCrc = 0;
        int32 compressToWidth = 0;
        int32 compressToHeight = 0;
        mutable uint32 convertedFileCrc = 0;

        void Clear();

        INTROSPECTION(Compression,
                      MEMBER(format, InspDesc("format", GlobalEnumMap<PixelFormat>::Instance()), I_VIEW | I_EDIT | I_SAVE)
                      MEMBER(imageFormat, InspDesc("format", GlobalEnumMap<ImageFormat>::Instance()), I_EDIT | I_SAVE)
                      MEMBER(sourceFileCrc, "Source File CRC", I_SAVE)
                      MEMBER(compressToWidth, "compressToWidth", I_SAVE)
                      MEMBER(compressToHeight, "compressToHeight", I_SAVE)
                      MEMBER(convertedFileCrc, "Converted File CRC", I_SAVE)
                      )
    };

public:
    TextureDescriptor();

    static TextureDescriptor* CreateFromFile(const FilePath& filePathname);
    static TextureDescriptor* CreateDescriptor(rhi::TextureAddrMode wrap, bool generateMipmaps);

    void Initialize(rhi::TextureAddrMode wrap, bool generateMipmaps);
    void Initialize(const TextureDescriptor* descriptor);
    bool Initialize(const FilePath& filePathname);

    void SetDefaultValues();

    void SetQualityGroup(const FastName& group);
    const FastName& GetQualityGroup() const;

    bool Load(const FilePath& filePathname); //may be protected?

    void Save() const;
    void Save(const FilePath& filePathname) const;
    void Export(const FilePath& filePathname, eGPUFamily forGPU) const;

    void OverridePathName(const FilePath& filename);

    bool IsCompressedTextureActual(eGPUFamily forGPU) const;
    bool HasCompressionFor(eGPUFamily forGPU) const;
    bool UpdateCrcForFormat(eGPUFamily forGPU) const;

    bool IsCompressedFile() const;
    void SetGenerateMipmaps(bool generateMipmaps);
    bool GetGenerateMipMaps() const;
    bool IsCubeMap() const;

    FilePath GetSourceTexturePathname() const;

    void GetFacePathnames(Vector<FilePath>& faceNames) const;
    void GenerateFacePathnames(const FilePath& baseName, const Array<String, Texture::CUBE_FACE_COUNT>& faceNameSuffixes, Vector<FilePath>& faceNames) const;

    static const String& GetDescriptorExtension();
    static const String& GetLightmapTextureExtension();
    static const String& GetDefaultFaceExtension();

    static bool IsSupportedTextureExtension(const String& extension);
    static bool IsSourceTextureExtension(const String& extension);
    static bool IsCompressedTextureExtension(const String& extension);
    static bool IsDescriptorExtension(const String& extension);

    static bool IsSupportedSourceFormat(ImageFormat imageFormat);
    static bool IsSupportedCompressedFormat(ImageFormat imageFormat);

    const String& GetFaceExtension(uint32 face) const;

    static FilePath GetDescriptorPathname(const FilePath& texturePathname);

    FilePath CreateMultiMipPathnameForGPU(const eGPUFamily forGPU) const;
    bool CreateSingleMipPathnamesForGPU(const eGPUFamily forGPU, Vector<FilePath>& pathes) const;
    void CreateLoadPathnamesForGPU(const eGPUFamily forGPU, Vector<FilePath>& pathes) const;

    PixelFormat GetPixelFormatForGPU(eGPUFamily forGPU) const;
    ImageFormat GetImageFormatForGPU(eGPUFamily forGPU) const;

    bool Reload();

    bool IsPresetValid(const KeyedArchive* presetArchive) const;
    bool DeserializeFromPreset(const KeyedArchive* presetArchive);
    bool SerializeToPreset(KeyedArchive* presetArchive) const;

private:
    const Compression* GetCompressionParams(eGPUFamily forGPU) const;

    //loading
    DAVA_DEPRECATED(void RecalculateCompressionSourceCRC());
    DAVA_DEPRECATED(uint32 ReadSourceCRC_V8_or_less() const);

    DAVA_DEPRECATED(void LoadVersion6(File* file));
    DAVA_DEPRECATED(void LoadVersion7(File* file));
    DAVA_DEPRECATED(void LoadVersion8(File* file));
    DAVA_DEPRECATED(void LoadVersion9(File* file));
    DAVA_DEPRECATED(void LoadVersion10(File* file));

    void LoadVersion11(File* file);
    void LoadVersion12(File* file);

    uint32 ReadSourceCRC() const;
    uint32 GetConvertedCRC(eGPUFamily forGPU) const;

    uint32 GenerateDescriptorCRC(eGPUFamily forGPU) const;

    void SaveInternal(File* file, const int32 signature, const eGPUFamily forGPU) const;

public:
    FilePath pathname;

    //moved from Texture
    FastName qualityGroup;
    TextureDrawSettings drawSettings;
    TextureDataSettings dataSettings;

    Compression compression[GPU_FAMILY_COUNT];

    static Array<ImageFormat, 6> sourceTextureTypes;
    static Array<ImageFormat, 2> compressedTextureTypes;

    eGPUFamily gpu = eGPUFamily::GPU_ORIGIN;
    ImageFormat imageFormat = ImageFormat::IMAGE_FORMAT_UNKNOWN;
    PixelFormat format = PixelFormat::FORMAT_INVALID; // texture format

    bool isCompressedFile = false;
};
};

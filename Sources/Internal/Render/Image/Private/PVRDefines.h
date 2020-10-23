#pragma once

#include "Base/BaseTypes.h"

namespace DAVA
{
/*****************************************************************************
* Texture related constants and enumerations. 
*****************************************************************************/
// V3 Header Identifiers.
const uint32 PVRTEX3_IDENT = 0x03525650; // 'P''V''R'3
const uint32 PVRTEX3_IDENT_REV = 0x50565203;
// If endianness is backwards then PVR3 will read as 3RVP, hence why it is written as an int.

//Current version texture identifiers
const uint32 PVRTEX_CURR_IDENT = PVRTEX3_IDENT;
const uint32 PVRTEX_CURR_IDENT_REV = PVRTEX3_IDENT_REV;

enum EPVRTColourSpace
{
    ePVRTCSpacelRGB,
    ePVRTCSpacesRGB,
    ePVRTCSpaceNumSpaces
};

//Compressed pixel formats
enum EPVRTPixelFormat
{
    ePVRTPF_PVRTCI_2bpp_RGB,
    ePVRTPF_PVRTCI_2bpp_RGBA,
    ePVRTPF_PVRTCI_4bpp_RGB,
    ePVRTPF_PVRTCI_4bpp_RGBA,
    ePVRTPF_PVRTCII_2bpp,
    ePVRTPF_PVRTCII_4bpp,
    ePVRTPF_ETC1,
    ePVRTPF_DXT1,
    ePVRTPF_DXT2,
    ePVRTPF_DXT3,
    ePVRTPF_DXT4,
    ePVRTPF_DXT5,

    //These formats are identical to some DXT formats.
    ePVRTPF_BC1 = ePVRTPF_DXT1,
    ePVRTPF_BC2 = ePVRTPF_DXT3,
    ePVRTPF_BC3 = ePVRTPF_DXT5,

    //These are currently unsupported:
    ePVRTPF_BC4,
    ePVRTPF_BC5,
    ePVRTPF_BC6,
    ePVRTPF_BC7,

    //These are supported
    ePVRTPF_UYVY,
    ePVRTPF_YUY2,
    ePVRTPF_BW1bpp,
    ePVRTPF_SharedExponentR9G9B9E5,
    ePVRTPF_RGBG8888,
    ePVRTPF_GRGB8888,
    ePVRTPF_ETC2_RGB,
    ePVRTPF_ETC2_RGBA,
    ePVRTPF_ETC2_RGB_A1,
    ePVRTPF_EAC_R11,
    ePVRTPF_EAC_RG11,

    //Invalid value
    ePVRTPF_NumCompressedPFs
};

//Variable Type Names
enum EPVRTVariableType
{
    ePVRTVarTypeUnsignedByteNorm,
    ePVRTVarTypeSignedByteNorm,
    ePVRTVarTypeUnsignedByte,
    ePVRTVarTypeSignedByte,
    ePVRTVarTypeUnsignedShortNorm,
    ePVRTVarTypeSignedShortNorm,
    ePVRTVarTypeUnsignedShort,
    ePVRTVarTypeSignedShort,
    ePVRTVarTypeUnsignedIntegerNorm,
    ePVRTVarTypeSignedIntegerNorm,
    ePVRTVarTypeUnsignedInteger,
    ePVRTVarTypeSignedInteger,
    ePVRTVarTypeSignedFloat,
    ePVRTVarTypeFloat = ePVRTVarTypeSignedFloat, //the name ePVRTVarTypeFloat is now deprecated.
    ePVRTVarTypeUnsignedFloat,
    ePVRTVarTypeNumVarTypes
};

//Generate a 4 channel PixelID.
#define PVRTGENPIXELID4(C1Name, C2Name, C3Name, C4Name, C1Bits, C2Bits, C3Bits, C4Bits) \
    ((static_cast<uint64>(C1Name)) + \
     (static_cast<uint64>(C2Name) << 8) + \
     (static_cast<uint64>(C3Name) << 16) + \
     (static_cast<uint64>(C4Name) << 24) + \
     (static_cast<uint64>(C1Bits) << 32) + \
     (static_cast<uint64>(C2Bits) << 40) + \
     (static_cast<uint64>(C3Bits) << 48) + \
     (static_cast<uint64>(C4Bits) << 56))

//Generate a 1 channel PixelID.
#define PVRTGENPIXELID3(C1Name, C2Name, C3Name, C1Bits, C2Bits, C3Bits) (PVRTGENPIXELID4(C1Name, C2Name, C3Name, 0, C1Bits, C2Bits, C3Bits, 0))

//Generate a 2 channel PixelID.
#define PVRTGENPIXELID2(C1Name, C2Name, C1Bits, C2Bits) (PVRTGENPIXELID4(C1Name, C2Name, 0, 0, C1Bits, C2Bits, 0, 0))

//Generate a 3 channel PixelID.
#define PVRTGENPIXELID1(C1Name, C1Bits) (PVRTGENPIXELID4(C1Name, 0, 0, 0, C1Bits, 0, 0, 0))

/*!***************************************************************************
 @Function		PVRTIsLittleEndian
 @Returns		True if the platform the code is ran on is little endian
 @Description	Returns true if the platform the code is ran on is little endian
 *****************************************************************************/
inline bool PVRTIsLittleEndian()
{
    static bool bLittleEndian;
    static bool bIsInit = false;

    if (!bIsInit)
    {
        short int word = 0x0001;
        char* byte = reinterpret_cast<char*>(&word);
        bLittleEndian = byte[0] ? true : false;
        bIsInit = true;
    }

    return bLittleEndian;
}

/*!***********************************************************************
 @struct       	MetaDataBlock
 @brief      	A struct containing a block of extraneous meta data for a texture.
 *************************************************************************/
struct MetaDataBlock
{
    uint32 DevFOURCC = 0; ///< A 4cc descriptor of the data type's creator. Values equating to values between 'P' 'V' 'R' 0 and 'P' 'V' 'R' 255 will be used by our headers.
    uint32 u32Key = 0; ///< A DWORD (enum value) identifying the data type, and thus how to read it.
    uint32 u32DataSize = 0; ///< Size of the Data member.
    uint8* Data = nullptr; ///< Data array, can be absolutely anything, the loader needs to know how to handle it based on DevFOURCC and Key. Use new operator to assign to it.

    /*!***********************************************************************
     @fn       		MetaDataBlock
     @brief      	Meta Data Block Constructor
     *************************************************************************/
    MetaDataBlock() = default;

    /*!***********************************************************************
     @fn       		MetaDataBlock
     @brief      	Meta Data Block Copy Constructor
     *************************************************************************/
    MetaDataBlock(const MetaDataBlock& rhs)
        : DevFOURCC(rhs.DevFOURCC)
        , u32Key(rhs.u32Key)
        , u32DataSize(rhs.u32DataSize)
    {
        //Copy the data across.
        Data = new uint8[u32DataSize];
        for (uint32 uiDataAmt = 0; uiDataAmt < u32DataSize; ++uiDataAmt)
        {
            Data[uiDataAmt] = rhs.Data[uiDataAmt];
        }
    }

    /*!***********************************************************************
     @fn       		~MetaDataBlock
     @brief      	Meta Data Block Destructor
     *************************************************************************/
    ~MetaDataBlock()
    {
        if (Data)
            delete[] Data;
        Data = nullptr;
    }
};
} //DAVA

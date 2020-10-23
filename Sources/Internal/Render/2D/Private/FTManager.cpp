#include "FTManager.h"
#include "Logger/Logger.h"
#include "FileSystem/File.h"
#include "FileSystem/FilePath.h"
#include "FileSystem/LocalizationSystem.h"
#include "UI/UIControlSystem.h"
#include "Render/2D/Font.h"

namespace DAVA
{
static const uint32 MAX_FACES = 4;
static const uint32 MAX_SIZES = 16;
static const uint32 MAX_BYTES = 2 * 1024 * 1024;

namespace FTFontDetails
{
FT_Error FaceRequester(FTC_FaceID face_id, FT_Library library, FT_Pointer request_data, FT_Face* aface)
{
    FTManager::FaceID* face = static_cast<FTManager::FaceID*>(face_id);
    return face->OpenFace(library, aface);
}
}

FTManager::FTManager()
{
    FT_Error error = FT_Init_FreeType(&library);
    if (error)
    {
        Logger::Error("FTManager: FT_Init_FreeType failed with error %d", error);
        DVASSERT(false, "FTManager: FT_Init_FreeType failed");
        return;
    }

    error = FTC_Manager_New(library, MAX_FACES, MAX_SIZES, MAX_BYTES, &FTFontDetails::FaceRequester, 0, &manager);
    if (error)
    {
        Logger::Error("FTManager: FTC_Manager_New failed with error %d", error);
        DVASSERT(false, "FTManager: FTC_Manager_New failed");
        return;
    }

    error = FTC_ImageCache_New(manager, &glyphcache);
    if (error)
    {
        Logger::Error("FTManager: FTC_ImageCache_New failed with error %d", error);
        DVASSERT(false, "FTManager: FTC_ImageCache_New failed");
    }

    error = FTC_CMapCache_New(manager, &cmapcache);
    if (error)
    {
        Logger::Error("FTManager: FTC_CMapCache_New failed with error %d", error);
        DVASSERT(false, "FTManager: FTC_CMapCache_New failed");
    }
}

FTManager::~FTManager()
{
    FTC_Manager_Done(manager);
    FT_Done_FreeType(library);
}

FT_Error FTManager::LookupFace(FaceID* faceId, FT_Face* face)
{
    return FTC_Manager_LookupFace(manager, static_cast<FTC_FaceID>(faceId), face);
}

FT_Error FTManager::LookupSize(FaceID* faceId, float32 size, FT_Size* ftsize)
{
    FTC_ScalerRec fontScaler =
    {
      static_cast<FTC_FaceID>(faceId),
      0,
      static_cast<FT_UInt>(size * 64.f) & -64, // 'floor' size value (it is better solution that 'round' inside FT)
      0,
      0,
      FT_UInt(Font::GetDPI())
    };
    return FTC_Manager_LookupSize(manager, &fontScaler, ftsize);
}

FT_Error FTManager::LookupGlyph(FaceID* faceId, float32 size, uint32 glyphIndex, FT_Glyph* glyph)
{
    FTC_ScalerRec fontScaler =
    {
      static_cast<FTC_FaceID>(faceId),
      0,
      static_cast<FT_UInt>(size * 64.f) & -64, // 'floor' size value (it is better solution that 'round' inside FT)
      0,
      0,
      FT_UInt(Font::GetDPI())
    };
    return FTC_ImageCache_LookupScaler(glyphcache, &fontScaler, FT_LOAD_DEFAULT | FT_LOAD_NO_HINTING, glyphIndex, glyph, 0);
}

uint32 FTManager::LookupGlyphIndex(FaceID* faceId, uint32 codePoint)
{
    return uint32(FTC_CMapCache_Lookup(cmapcache, static_cast<FTC_FaceID>(faceId), 0, static_cast<FT_UInt32>(codePoint)));
}

void FTManager::RemoveFace(FaceID* faceId)
{
    FTC_Manager_RemoveFaceID(manager, static_cast<FTC_FaceID>(faceId));
}

} // DAVA

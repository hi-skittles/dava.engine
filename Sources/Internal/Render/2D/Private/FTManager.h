#pragma once

#include "Base/BaseTypes.h"
#include "FTInclude.h"

namespace DAVA
{
/** Class to work with FreeType cache managers. */
class FTManager final
{
public:
    /**
    Interface for load faces from data or files.
    Also using for indexing faces in FreeType cache managers.
    */
    struct FaceID
    {
        /**
        Open face from memory/file and set loaded FreeType face to `ftface`.
        Return 0 if successful.
        */
        virtual FT_Error OpenFace(FT_Library library, FT_Face* ftface) = 0;

        /** Default destructor. */
        virtual ~FaceID() = default;
    };

    /** Contruct FreeType cache managers. */
    FTManager();

    /** Free FreeType cache managers. */
    ~FTManager();

    /**
    Get FreeType face from manager.
    Return 0 if successful.
    */
    FT_Error LookupFace(FaceID* faceId, FT_Face* face);

    /**
    Get FreeType size structure with specified `faceId` and `size`.
    Return 0 if successful.
    */
    FT_Error LookupSize(FaceID* faceId, float32 size, FT_Size* ftsize);

    /**
    Get FreeType glyph from manager.
    Return 0 if successful.
    */
    FT_Error LookupGlyph(FaceID* faceId, float32 size, uint32 glyphIndex, FT_Glyph* glyph);

    /**
    Get glyph index in face for specified codePoint.
    Return 0 if glyph not found.
    */
    uint32 LookupGlyphIndex(FaceID* faceId, uint32 codePoint);

    /**
    Delete stored face from cache.
    */
    void RemoveFace(FaceID* faceId);

private:
    FT_Library library = nullptr;
    FTC_Manager manager = nullptr;
    FTC_ImageCache glyphcache = nullptr;
    FTC_CMapCache cmapcache = nullptr;
};

} // DAVA

#ifndef __DISTANCE_GENERATOR__TTFFONT__
#define __DISTANCE_GENERATOR__TTFFONT__

#include <Base/BaseTypes.h>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H

class TtfFont
{
public:
    TtfFont();
    ~TtfFont();

    bool Init(const DAVA::String& file);
    void DeInit();

    void SetSize(DAVA::int32 size);
    DAVA::int32 GetSize() const;

    bool SetCharMap(DAVA::int32 charmap);
    DAVA::int32 GetCharMap() const;

    DAVA::float32 GetLineHeight();
    DAVA::float32 GetBaseline() const;

    FT_Face& GetFace();

protected:
    DAVA::int32 size;
    FT_Face face;
    bool faceInit;
    DAVA::String fileName;
    bool libraryInit;
    FT_Library library;
};

#endif /* defined(__DISTANCE_GENERATOR__TTFFONT__) */

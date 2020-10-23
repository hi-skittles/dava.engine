#include "TtfFont.h"

using namespace std;
using namespace DAVA;

TtfFont::TtfFont()
    : faceInit(false)
    , libraryInit(false)
    , size(32)
{
    auto err = FT_Init_FreeType(&library);
    if (err)
    {
        printf("Library init error");
    }
    else
    {
        libraryInit = true;
    }
}

TtfFont::~TtfFont()
{
    DeInit();
    if (libraryInit)
    {
        FT_Done_FreeType(library);
        libraryInit = false;
    }
}

bool TtfFont::Init(const String& file)
{
    DeInit();

    if (!libraryInit)
    {
        return false;
    }

    auto res = FT_New_Face(library, file.c_str(), 0, &face);
    if (res)
    {
        faceInit = false;
        printf("%s: init error\n", file.c_str());
        return false;
    }

    res = FT_Set_Pixel_Sizes(face, size, 0);
    if (res)
    {
        faceInit = false;
        printf("%s: init error\n", file.c_str());
        return false;
    }

    faceInit = true;
    fileName = file;

    return true;
}

void TtfFont::DeInit()
{
    if (faceInit)
    {
        FT_Done_Face(face);
        faceInit = false;
    }
}

void TtfFont::SetSize(int32 _size)
{
    FT_Set_Pixel_Sizes(face, 0, _size);
    size = _size;
}

int32 TtfFont::GetSize() const
{
    return size;
}

FT_Face& TtfFont::GetFace()
{
    return face;
}

float32 TtfFont::GetLineHeight()
{
    return FT_MulFix(face->bbox.yMax - face->bbox.yMin, face->size->metrics.y_scale) / 64.f;
}

float32 TtfFont::GetBaseline() const
{
    return FT_MulFix(face->bbox.yMax, face->size->metrics.y_scale) / 64.f;
}

bool TtfFont::SetCharMap(int32 charmap)
{
    if (charmap >= 0 && charmap < face->num_charmaps)
    {
        auto res = FT_Set_Charmap(face, face->charmaps[charmap]);
        if (res == 0)
        {
            return true;
        }
    }

    return false;
}

int32 TtfFont::GetCharMap() const
{
    for (auto i = 0; i < face->num_charmaps; ++i)
    {
        if (face->charmap == face->charmaps[i])
        {
            return i;
        }
    }

    return -1;
}

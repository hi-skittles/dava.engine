#include "Render/2D/FTFont.h"
#include "Debug/DVAssert.h"
#include "Engine/Engine.h"
#include "FileSystem/File.h"
#include "FileSystem/FilePath.h"
#include "FileSystem/LocalizationSystem.h"
#include "FileSystem/YamlNode.h"
#include "FileSystem/YamlParser.h"
#include "Logger/Logger.h"
#include "Render/2D/FontManager.h"
#include "Render/2D/Private/FTManager.h"
#include "Render/2D/Systems/VirtualCoordinatesSystem.h"
#include "Render/Renderer.h"
#include "UI/UIControlSystem.h"
#include "Utils/UTF8Utils.h"

namespace DAVA
{
#ifdef USE_FILEPATH_IN_MAP
using FontMap = Map<FilePath, FTInternalFont*>;
#else //#ifdef USE_FILEPATH_IN_MAP
using FontMap = Map<String, FTInternalFont*>;
#endif //#ifdef USE_FILEPATH_IN_MAP
FontMap fontMap;

class FTInternalFont : public BaseObject, public FTManager::FaceID
{
    friend class FTFont;

private:
    FTInternalFont(const FilePath& path);
    virtual ~FTInternalFont();

public:
    Font::StringMetrics DrawString(const WideString& str, void* buffer, int32 bufWidth, int32 bufHeight,
                                   uint8 r, uint8 g, uint8 b, uint8 a,
                                   float32 size, bool realDraw,
                                   int32 offsetX, int32 offsetY,
                                   int32 justifyWidth, int32 spaceAddon,
                                   float32 ascendScale, float32 descendScale,
                                   Vector<float32>* charSizes = NULL,
                                   bool contentScaleIncluded = false);
    uint32 GetFontHeight(float32 size, float32 ascendScale, float32 descendScale);
    bool IsCharAvaliable(char16 ch);

    // FaceID methods
    FT_Error OpenFace(FT_Library library, FT_Face* ftface) override;

private:
    FTManager* ftm = nullptr;
    FilePath fontPath;
    FT_StreamRec stream;

    struct Glyph
    {
        FT_UInt index = 0;
        FT_Glyph image = nullptr; /* the glyph image */
        FT_Pos delta = 0; /* delta caused by hinting */

        bool operator<(const Glyph& right) const
        {
            return image < right.image;
        };
    };
    Vector<Glyph> glyphs;

    bool initialized = false;

    void ClearString();
    int32 LoadString(float32 size, const WideString& str);
    void Prepare(FT_Face face, FT_Vector* advances);

    inline int32 FtRound(int32 val);
    inline int32 FtCeil(int32 val);

    static Mutex drawStringMutex;
    static const int32 ftToPixelShift; // Int value for shift to convert FT point to pixel
    static const float32 ftToPixelScale; // Float value to convert FT point to pixel
};

const int32 FTInternalFont::ftToPixelShift = 6;
const float32 FTInternalFont::ftToPixelScale = 1.f / 64.f;

////////////////////////////////////////////////////////////////////////////////

namespace FTFontDetails
{
unsigned long StreamLoad(FT_Stream stream, unsigned long offset, unsigned char* buffer, unsigned long count)
{
    File* is = reinterpret_cast<File*>(stream->descriptor.pointer);
    if (count == 0)
        return 0;
    is->Seek(int32(offset), File::SEEK_FROM_START);
    return is->Read(buffer, uint32(count));
}

void StreamClose(FT_Stream stream)
{
    File* file = reinterpret_cast<File*>(stream->descriptor.pointer);
    SafeRelease(file);
}
}

////////////////////////////////////////////////////////////////////////////////

FTFont::FTFont(FTInternalFont* _internalFont)
{
    internalFont = SafeRetain(_internalFont);
    fontType = TYPE_FT;
}

FTFont::~FTFont()
{
    SafeRelease(internalFont);
}

FTFont* FTFont::Create(const FilePath& path)
{
    FTInternalFont* iFont = 0;

    FontMap::iterator it = fontMap.find(path);
    if (it != fontMap.end())
    {
        iFont = it->second;
    }

    if (!iFont)
    {
        iFont = new FTInternalFont(path);
        fontMap[FILEPATH_MAP_KEY(path)] = iFont;
    }

    FTFont* font = new FTFont(iFont);
    font->fontPath = path;

    return font;
}

void FTFont::ClearCache()
{
    while (fontMap.size())
    {
        SafeRelease(fontMap.begin()->second);
        fontMap.erase(fontMap.begin());
    }
}

FTFont* FTFont::Clone() const
{
    FTFont* retFont = new FTFont(internalFont);
    retFont->verticalSpacing = verticalSpacing;
    retFont->ascendScale = ascendScale;
    retFont->descendScale = descendScale;
    retFont->fontPath = fontPath;
    return retFont;
}

bool FTFont::IsEqual(const Font* font) const
{
    if (font->GetFontType() != this->GetFontType())
    {
        return false;
    }

    const FTFont* ftfont = DynamicTypeCheck<const FTFont*>(font);
    if (!Font::IsEqual(font) || internalFont != ftfont->internalFont)
    {
        return false;
    }

    return true;
}

String FTFont::GetRawHashString()
{
    return fontPath.GetFrameworkPath() + "_" + Font::GetRawHashString();
}

bool FTFont::IsTextSupportsSoftwareRendering() const
{
    return true;
};

Font::StringMetrics FTFont::DrawStringToBuffer(float32 size, void* buffer, int32 bufWidth, int32 bufHeight, int32 offsetX, int32 offsetY, int32 justifyWidth, int32 spaceAddon, const WideString& str, bool contentScaleIncluded)
{
    return internalFont->DrawString(str, buffer, bufWidth, bufHeight, 255, 255, 255, 255, size, true, offsetX, offsetY, justifyWidth, spaceAddon, ascendScale, descendScale, NULL, contentScaleIncluded);
}

Font::StringMetrics FTFont::GetStringMetrics(float32 size, const WideString& str, Vector<float32>* charSizes) const
{
    if (charSizes != nullptr)
    {
        charSizes->clear();
    }
    return internalFont->DrawString(str, 0, 0, 0, 0, 0, 0, 0, size, false, 0, 0, 0, 0, ascendScale, descendScale, charSizes);
}

uint32 FTFont::GetFontHeight(float32 size) const
{
    return internalFont->GetFontHeight(size, ascendScale, descendScale);
}

bool FTFont::IsCharAvaliable(char16 ch) const
{
    return internalFont->IsCharAvaliable(ch);
}

const FilePath& FTFont::GetFontPath() const
{
    return internalFont->fontPath;
}

YamlNode* FTFont::SaveToYamlNode() const
{
    YamlNode* node = Font::SaveToYamlNode();
    //Type
    node->Set("type", "FTFont");

    String pathname = internalFont->fontPath.GetFrameworkPath();
    node->Set("name", pathname);

    return node;
}

void FTFont::SetAscendScale(float32 _ascendScale)
{
    ascendScale = _ascendScale;
}

DAVA::float32 FTFont::GetAscendScale() const
{
    return ascendScale;
}

void FTFont::SetDescendScale(float32 _descendScale)
{
    descendScale = _descendScale;
}

DAVA::float32 FTFont::GetDescendScale() const
{
    return descendScale;
}

////////////////////////////////////////////////////////////////////////////////

Mutex FTInternalFont::drawStringMutex;

/**
 /brief Wrap around FT_MulFix, because this function is written in assembler and
        during optimization beside her badly generated machine code.
        ALWAYS USE THIS FUNCTION INSTEAD FT_MulFix!
 */
#if defined(__DAVAENGINE_IPHONE__) || defined(__DAVAENGINE_MACOS__)
static FT_Long FT_MulFix_Wrapper(FT_Long a, FT_Long b) __attribute__((noinline));
#else
static FT_Long FT_MulFix_Wrapper(FT_Long a, FT_Long b);
#endif

FT_Long FT_MulFix_Wrapper(FT_Long a, FT_Long b)
{
    return FT_MulFix(a, b);
}

FTInternalFont::FTInternalFont(const FilePath& path)
    : fontPath(path)
{
    ftm = GetEngineContext()->fontManager->GetFT();
    DVASSERT(ftm);

    FT_Face face = nullptr;
    FT_Error error = ftm->LookupFace(this, &face);
    initialized = (error == FT_Err_Ok && face != nullptr);
}

FTInternalFont::~FTInternalFont()
{
    ClearString();
    ftm->RemoveFace(this);
}

FT_Error FTInternalFont::OpenFace(FT_Library library, FT_Face* ftface)
{
    FilePath localizedPath(fontPath);
    localizedPath.ReplaceDirectory(fontPath.GetDirectory() + (LocalizationSystem::Instance()->GetCurrentLocale() + "/"));

    File* fontFile = File::Create(localizedPath, File::READ | File::OPEN); // try to open localized font
    if (!fontFile)
    {
        fontFile = File::Create(fontPath, File::READ | File::OPEN); // try to open base font
        if (!fontFile)
        {
            Logger::Error("Failed to open font: %s", fontPath.GetStringValue().c_str());
            return FT_Err_Cannot_Open_Resource;
        }
    }

    stream.base = 0;
    stream.size = static_cast<uint32>(fontFile->GetSize());
    stream.pos = 0;
    stream.descriptor.pointer = static_cast<void*>(fontFile);
    stream.pathname.pointer = 0;
    stream.read = &FTFontDetails::StreamLoad;
    stream.close = &FTFontDetails::StreamClose;
    stream.memory = 0;
    stream.cursor = 0;
    stream.limit = 0;

    FT_Open_Args args;
    args.flags = FT_OPEN_STREAM;
    args.memory_base = 0;
    args.memory_size = 0;
    args.pathname = 0;
    args.stream = &stream;
    args.driver = 0;
    args.num_params = 0;
    args.params = 0;

    FT_Error error = FT_Open_Face(library, &args, 0, ftface);
    if (error == FT_Err_Unknown_File_Format)
    {
        Logger::Error("FTInternalFont::FTInternalFont FT_Err_Unknown_File_Format: %s", fontPath.GetStringValue().c_str());
    }
    else if (error)
    {
        Logger::Error("FTInternalFont::FTInternalFont cannot create font: %s", fontPath.GetStringValue().c_str());
    }
    return error;
}

Font::StringMetrics FTInternalFont::DrawString(const WideString& str, void* buffer, int32 bufWidth, int32 bufHeight,
                                               uint8 r, uint8 g, uint8 b, uint8 a,
                                               float32 size, bool realDraw,
                                               int32 offsetX, int32 offsetY,
                                               int32 justifyWidth, int32 spaceAddon,
                                               float32 ascendScale, float32 descendScale,
                                               Vector<float32>* charSizes,
                                               bool contentScaleIncluded)
{
    if (!initialized)
    {
        if (charSizes)
        {
            charSizes->assign(str.length(), 0.f);
        }
        return Font::StringMetrics();
    }

    drawStringMutex.Lock();

    bool drawNondefGlyph = Renderer::GetOptions()->IsOptionEnabled(RenderOptions::DRAW_NONDEF_GLYPH);

    size = GetEngineContext()->uiControlSystem->vcs->ConvertVirtualToPhysicalY(size); // increase size for high dpi screens
    if (!contentScaleIncluded)
    {
        bufWidth = int32(GetEngineContext()->uiControlSystem->vcs->ConvertVirtualToPhysicalX(float32(bufWidth)));
        bufHeight = int32(GetEngineContext()->uiControlSystem->vcs->ConvertVirtualToPhysicalY(float32(bufHeight)));
        offsetY = int32(GetEngineContext()->uiControlSystem->vcs->ConvertVirtualToPhysicalY(float32(offsetY)));
        offsetX = int32(GetEngineContext()->uiControlSystem->vcs->ConvertVirtualToPhysicalX(float32(offsetX)));
    }

    FT_Size ft_size = nullptr;
    FT_Error error = ftm->LookupSize(this, size, &ft_size);

    if (error != FT_Err_Ok)
    {
        Logger::Error("[FTInternalFont::DrawString] LookupSize error %d", error);
        return Font::StringMetrics();
    }

    int32 faceBboxYMin = int32(FT_MulFix_Wrapper(ft_size->face->bbox.yMin, ft_size->metrics.y_scale) * descendScale); // draw offset
    int32 faceBboxYMax = int32(FT_MulFix_Wrapper(ft_size->face->bbox.yMax, ft_size->metrics.y_scale) * ascendScale); // baseline

    FT_Vector pen;
    pen.x = offsetX << ftToPixelShift;
    pen.y = offsetY << ftToPixelShift;
    pen.y -= FT_Pos(faceBboxYMin); //bring baseline up

    int32 countSpace = LoadString(size, str);
    uint32 strLen = uint32(str.length());
    FT_Vector* advances = new FT_Vector[strLen];
    Prepare(ft_size->face, advances);

    float32 baseSize = (faceBboxYMax - faceBboxYMin) * ftToPixelScale;
    int32 multilineOffsetY = int32(std::ceil(baseSize)) + offsetY * 2;

    int32 justifyOffset = 0;
    int32 fixJustifyOffset = 0;
    if (countSpace > 0 && justifyWidth > 0 && spaceAddon > 0)
    {
        int32 diff = justifyWidth - spaceAddon;
        justifyOffset = diff / countSpace;
        fixJustifyOffset = diff - justifyOffset * countSpace;
    }

    Font::StringMetrics metrics;
    metrics.baseline = faceBboxYMax * ftToPixelScale;
    metrics.height = baseSize;
    metrics.drawRect = Rect2i(0x7fffffff, 0x7fffffff, 0, int32(std::ceil(baseSize))); // Setup rect with maximum int32 value for x/y, and zero width

    int32 layoutWidth = 0; // width in FT points

    for (uint32 i = 0; i < strLen; ++i)
    {
        Glyph& glyph = glyphs[i];
        FT_Glyph image = nullptr;
        FT_BBox bbox;

        bool skipGlyph = true;
        if (glyph.image && (glyph.index != 0 || drawNondefGlyph))
        {
            error = FT_Glyph_Copy(glyph.image, &image);
            if (error == 0)
            {
                // Make justify offsets only for visible glyphs
                if (i > 0 && (justifyOffset > 0 || fixJustifyOffset > 0))
                {
                    if (str[i - 1] == L' ')
                    {
                        advances[i].x += justifyOffset << ftToPixelShift; //Increase advance of character
                    }
                    if (fixJustifyOffset > 0)
                    {
                        fixJustifyOffset--;
                        advances[i].x += 1 << ftToPixelShift; //Increase advance of character
                    }
                }

                error = FT_Glyph_Transform(image, nullptr, &pen);
                if (error == 0)
                {
                    FT_Glyph_Get_CBox(image, FT_GLYPH_BBOX_PIXELS, &bbox);
                    if (realDraw)
                    {
                        error = FT_Glyph_To_Bitmap(&image, FT_RENDER_MODE_NORMAL, 0, 1);
                    }
                }
            }
            skipGlyph = error != 0;
        }

        if (skipGlyph)
        {
            // Add zero char size for invalid glyph
            if (charSizes)
            {
                charSizes->push_back(0.f);
            }
        }
        else
        {
            if (charSizes)
            {
                float32 charSize = float32(advances[i].x) * ftToPixelScale; // Convert to pixels
                charSize = GetEngineContext()->uiControlSystem->vcs->ConvertPhysicalToVirtualX(charSize); // Convert to virtual space
                charSizes->push_back(charSize);
            }

            layoutWidth += advances[i].x;

            int32 width = 0;
            int32 height = 0;
            int32 left = 0;
            int32 top = 0;
            if (glyph.index > 0)
            {
                metrics.drawRect.x = Min(metrics.drawRect.x, int32(bbox.xMin));
                metrics.drawRect.y = Min(metrics.drawRect.y, multilineOffsetY - int32(bbox.yMax));
                metrics.drawRect.dx = Max(metrics.drawRect.dx, int32(bbox.xMax));
                metrics.drawRect.dy = Max(metrics.drawRect.dy, multilineOffsetY - int32(bbox.yMin));
            }
            else // guess bitmap dimensions for empty bitmap
            {
                width = int32(advances[i].x) >> ftToPixelShift;
                height = int32(std::ceil(2 * metrics.baseline - metrics.height));
                left = int32(pen.x) >> ftToPixelShift;
                top = multilineOffsetY - (int32(pen.y) >> ftToPixelShift) - height;

                metrics.drawRect.x = Min(metrics.drawRect.x, left);
                metrics.drawRect.y = Min(metrics.drawRect.y, top);
                metrics.drawRect.dx = Max(metrics.drawRect.dx, left + width);
                metrics.drawRect.dy = Max(metrics.drawRect.dy, top + height);
            }

            if (realDraw && bbox.xMin < bufWidth && bbox.yMin < bufHeight)
            {
                FT_BitmapGlyph bit = FT_BitmapGlyph(image);
                FT_Bitmap* bitmap = &bit->bitmap;

                if (glyph.index > 0)
                {
                    left = bit->left;
                    top = multilineOffsetY - bit->top;
                    width = bitmap->width;
                    height = bitmap->rows;
                }

                if (top >= 0 && left >= 0)
                {
                    uint8* resultBuf = static_cast<uint8*>(buffer);
                    int32 realH = Min(height, bufHeight - top);
                    int32 realW = Min(width, bufWidth - left);
                    int32 ind = top * bufWidth + left;
                    DVASSERT(((ind >= 0) && (ind < bufWidth * bufHeight)) || (realW * realH == 0));
                    uint8* writeBuf = resultBuf + ind;

                    if (glyph.index == 0)
                    {
                        for (int32 h = 0; h < realH; h++)
                        {
                            for (int32 w = 0; w < realW; w++)
                            {
                                if (w == 0 || w == realW - 1 || h == 0 || h == realH - 1)
                                    *writeBuf++ = 255;
                                else
                                    *writeBuf++ = 0;
                            }
                            writeBuf += bufWidth - realW;
                        }
                    }
                    else
                    {
                        uint8* readBuf = bitmap->buffer;
                        for (int32 h = 0; h < realH; h++)
                        {
                            for (int32 w = 0; w < realW; w++)
                            {
                                *writeBuf++ |= *readBuf++;
                            }
                            writeBuf += bufWidth - realW;
                            // DF-1827 - Increment read buffer with proper value
                            readBuf += width - realW;
                        }
                    }

                    if (writeBuf > resultBuf + ind)
                    {
                        DVASSERT((writeBuf - resultBuf - (bufWidth - realW)) <= (bufWidth * bufHeight));
                    }
                }
            }

            pen.x += advances[i].x;
            pen.y += advances[i].y;
        }

        if (image)
        {
            FT_Done_Glyph(image);
        }
    }

    SafeDeleteArray(advances);
    drawStringMutex.Unlock();

    if (metrics.drawRect.x == 0x7fffffff || metrics.drawRect.y == 0x7fffffff) // Empty string
    {
        metrics.drawRect.x = 0;
        metrics.drawRect.y = 0;
    }

    // Transform right/bottom edges into width/height
    metrics.drawRect.dx += -metrics.drawRect.x + 1;
    metrics.drawRect.dy += -metrics.drawRect.y + 1;

    // Transform width from FT points to pixels
    float32 totalWidth = float32(layoutWidth) * ftToPixelScale;

    if (!contentScaleIncluded)
    {
        metrics.drawRect.x = int32(std::floor(GetEngineContext()->uiControlSystem->vcs->ConvertPhysicalToVirtualX(float32(metrics.drawRect.x))));
        metrics.drawRect.y = int32(std::floor(GetEngineContext()->uiControlSystem->vcs->ConvertPhysicalToVirtualY(float32(metrics.drawRect.y))));
        metrics.drawRect.dx = int32(std::ceil(GetEngineContext()->uiControlSystem->vcs->ConvertPhysicalToVirtualX(float32(metrics.drawRect.dx))));
        metrics.drawRect.dy = int32(std::ceil(GetEngineContext()->uiControlSystem->vcs->ConvertPhysicalToVirtualY(float32(metrics.drawRect.dy))));
        metrics.baseline = GetEngineContext()->uiControlSystem->vcs->ConvertPhysicalToVirtualX(metrics.baseline);
        metrics.height = GetEngineContext()->uiControlSystem->vcs->ConvertPhysicalToVirtualY(metrics.height);
        metrics.width = GetEngineContext()->uiControlSystem->vcs->ConvertPhysicalToVirtualX(totalWidth);
    }
    else
    {
        metrics.width = totalWidth;
    }
    return metrics;
}

bool FTInternalFont::IsCharAvaliable(char16 ch)
{
    if (!initialized)
    {
        return false;
    }

    uint32 index = ftm->LookupGlyphIndex(this, ch);
    return index != 0;
}

uint32 FTInternalFont::GetFontHeight(float32 size, float32 ascendScale, float32 descendScale)
{
    if (!initialized)
    {
        return 0;
    }

    size = GetEngineContext()->uiControlSystem->vcs->ConvertVirtualToPhysicalY(size); // increase size for high dpi screens
    FT_Size ft_size = nullptr;
    if (ftm->LookupSize(this, size, &ft_size) == FT_Err_Ok)
    {
        int32 faceBboxYMin = int32(FT_MulFix_Wrapper(ft_size->face->bbox.yMin, ft_size->metrics.y_scale) * descendScale); // draw offset
        int32 faceBboxYMax = int32(FT_MulFix_Wrapper(ft_size->face->bbox.yMax, ft_size->metrics.y_scale) * ascendScale); // baseline
        float32 height = std::ceil((faceBboxYMax - faceBboxYMin) * ftToPixelScale);
        return uint32(std::ceil(GetEngineContext()->uiControlSystem->vcs->ConvertPhysicalToVirtualX(height))); // cover height to virtual coordinates back
    }
    return 0;
}

void FTInternalFont::Prepare(FT_Face face, FT_Vector* advances)
{
    if (!initialized)
    {
        return;
    }

    FT_Vector* prevAdvance = 0;
    FT_UInt prevIndex = 0;
    const bool useKerning = (FT_HAS_KERNING(face) > 0);
    const uint32 size = uint32(glyphs.size());

    for (uint32 i = 0; i < size; ++i)
    {
        Glyph& glyph = glyphs[i];

        advances[i] = glyph.image->advance;
        advances[i].x >>= 10; // Translate advances in
        advances[i].y >>= 10; // 16.16 to 26.6 format

        if (prevAdvance)
        {
            if (useKerning)
            {
                FT_Vector kern;
                FT_Get_Kerning(face, prevIndex, glyph.index, FT_KERNING_UNFITTED, &kern);
                // Scale kerning from virtual to physical, because FT_Set_Transform
                // converts only glyph advances without kerning.
                // It used for mobile platforms with different DPI and scale factor (iOS/Android).
                // See http://www.freetype.org/freetype2/docs/reference/ft2-base_interface.html#FT_Set_Transform
                prevAdvance->x += kern.x;
                prevAdvance->y += kern.y;
                prevAdvance->x += glyph.delta;
            }
        }
        prevIndex = glyph.index;
        prevAdvance = &advances[i];
    }
}

void FTInternalFont::ClearString()
{
    glyphs.clear();
}

int32 FTInternalFont::LoadString(float32 size, const WideString& str)
{
    ClearString();

    int32 spacesCount = 0;
    uint32 count = uint32(str.size());
    for (uint32 i = 0; i < count; ++i)
    {
        if (L' ' == str[i])
        {
            spacesCount++;
        }

        Glyph glyph;
        glyph.index = ftm->LookupGlyphIndex(this, str[i]);
        FT_Error error = ftm->LookupGlyph(this, size, glyph.index, &glyph.image);
        if (error != FT_Err_Ok)
        {
#if defined(__DAVAENGINE_DEBUG__)
            // DVASSERT(false); //This situation can be unnormal. Check it
            Logger::Warning("[FTInternalFont::LoadString] LookupGlyph error %d, str = %s", error, UTF8Utils::EncodeToUTF8(str).c_str());
#endif //__DAVAENGINE_DEBUG__
        }

        glyphs.push_back(glyph);
    }

#if defined(__DAVAENGINE_DEBUG__)
// Set<Glyph> tmp;
// tmp.insert(glyphs.begin(), glyphs.end());
// DVASSERT(tmp.size() == glyphs.size()); //This situation can be unnormal. Check it
#endif //__DAVAENGINE_DEBUG__

    return spacesCount;
}

int32 FTInternalFont::FtRound(int32 val)
{
    return (((val) + 32) & -64);
}

inline int32 FTInternalFont::FtCeil(int32 val)
{
    return (((val) + 63) & -64);
}
};

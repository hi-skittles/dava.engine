#include <Base/BaseTypes.h>

class TtfFont;

class FontConvertor
{
public:
    enum eModes
    {
        MODE_INVALID = -1,
        MODE_GENERATE = 0,
        MODE_ADJUST_FONT,
        MODE_ADJUST_TEXTURE,
        MODES_COUNT
    };

    enum eOutputType
    {
        TYPE_INVALID = -1,
        TYPE_GRAPHIC = 0,
        TYPE_DISTANCE_FIELD,
        TYPES_COUNT
    };

    struct Params
    {
        eModes mode;
        eOutputType output;
        DAVA::String filename;
        DAVA::String charListFile;
        DAVA::int32 maxChar;
        DAVA::int32 spread;
        DAVA::int32 scale;
        DAVA::int32 fontSize;
        DAVA::int32 textureSize;
        DAVA::int32 charmap;

        Params();
        static Params GetDefault();
    };

    static const DAVA::int32 MAX_TEXTURE_SIZE = 4096;
    static const DAVA::int32 MIN_TEXTURE_SIZE = 64;

    FontConvertor();
    ~FontConvertor();

    void InitWithParams(Params params);

    static eModes ModeFromString(const DAVA::String& str);
    static eOutputType TypeFromString(const DAVA::String& str);

    bool Convert();

private:
    struct CharDescription
    {
        DAVA::int32 id;
        DAVA::int32 x;
        DAVA::int32 y;
        DAVA::int32 width;
        DAVA::int32 height;
        DAVA::float32 xOffset;
        DAVA::float32 yOffset;
        DAVA::float32 xAdvance;
        DAVA::Map<DAVA::int32, DAVA::float32> kernings;
    };

private:
    Params params;
    TtfFont* font;
    DAVA::Map<DAVA::int32, CharDescription> chars;
    DAVA::Vector<std::pair<DAVA::int32, DAVA::int32>> charGlyphPairs;
    DAVA::int32 kerningCount;

    void SetDefaultParams();
    bool FillCharList();
    void FillKerning();
    void LoadCharList(DAVA::Vector<DAVA::int32>& charList);

    bool GeneratePackedList(DAVA::int32 fontSize, DAVA::int32 textureSize);
    void GenerateOutputImage();
    void GenerateFontDescription();

    DAVA::uint8* BuildDistanceField(const DAVA::uint8* inputBuf, DAVA::int32 inWidth, DAVA::int32 inHeight);
    DAVA::float32 FindSignedDistance(DAVA::int32 centerX, DAVA::int32 centerY, const DAVA::uint8* inputBuf, DAVA::int32 width, DAVA::int32 height);
    DAVA::uint8 DistanceToAlpha(DAVA::float32 distance);

    bool AdjustFontSize(DAVA::int32& size);
    bool AdjustTextureSize(DAVA::int32& size);
};
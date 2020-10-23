#include "FontConvertor.h"
#include "TtfFont.h"

#include "BinPacker/BinPacker.hpp"
#include "LodePng/lodepng.h"

#include <Utils/UTF8Utils.h>

#include <assert.h>
#include <iostream>
#include <iomanip>
#include <fstream>

using namespace std;
using namespace DAVA;

static const uint32 NOT_DEF_CHAR = 0xffff;
static const uint32 NOT_DEF_REPLACEMENT_ASCII_CODE = 32; // Space

FontConvertor::Params::Params()
    : filename("")
    , maxChar(-1)
    , charListFile("")
    , spread(-1)
    , scale(-1)
    , mode(FontConvertor::MODE_INVALID)
    , fontSize(-1)
    , textureSize(-1)
    , charmap(-1)
    , output(TYPE_DISTANCE_FIELD)
{
}

FontConvertor::Params FontConvertor::Params::GetDefault()
{
    Params params;
    params.mode = MODE_GENERATE;
    params.output = TYPE_DISTANCE_FIELD;
    params.fontSize = 32;
    params.maxChar = 128;
    params.scale = 16;
    params.spread = 2;
    params.textureSize = 512;
    params.charmap = 0;

    return params;
}

FontConvertor::eModes FontConvertor::ModeFromString(const String& str)
{
    if (str == "generate")
    {
        return FontConvertor::MODE_GENERATE;
    }
    else if (str == "adjustfont")
    {
        return FontConvertor::MODE_ADJUST_FONT;
    }
    else if (str == "adjusttexture")
    {
        return FontConvertor::MODE_ADJUST_TEXTURE;
    }

    return FontConvertor::MODE_INVALID;
}

FontConvertor::eOutputType FontConvertor::TypeFromString(const String& str)
{
    if (str == "gf")
    {
        return FontConvertor::TYPE_GRAPHIC;
    }
    else if (str == "dff")
    {
        return FontConvertor::TYPE_DISTANCE_FIELD;
    }

    return FontConvertor::TYPE_INVALID;
}

FontConvertor::FontConvertor()
    : font(new TtfFont())
{
    SetDefaultParams();
}

FontConvertor::~FontConvertor()
{
    SafeDelete(font);
}

void FontConvertor::InitWithParams(Params _params)
{
    auto def = Params::GetDefault();

    if (_params.fontSize <= 0)
    {
        _params.fontSize = def.fontSize;
    }
    if (_params.maxChar <= 0)
    {
        _params.maxChar = def.maxChar;
    }
    if (_params.mode <= MODE_INVALID || _params.mode >= MODES_COUNT)
    {
        _params.mode = def.mode;
    }
    if (_params.scale <= 0)
    {
        _params.scale = def.scale;
    }
    if (_params.spread < 0)
    {
        _params.spread = def.spread;
    }
    if (_params.textureSize < MIN_TEXTURE_SIZE || _params.textureSize > MAX_TEXTURE_SIZE)
    {
        _params.textureSize = def.textureSize;
    }
    if (_params.charmap < 0)
    {
        _params.charmap = def.charmap;
    }
    if (_params.output <= TYPE_INVALID || _params.output >= TYPES_COUNT)
    {
        _params.output = def.output;
    }

    params = _params;

    if (params.output == TYPE_GRAPHIC)
    {
        // Scale not supported for graphics font
        // We'll draw glyph form font as is
        params.scale = 1;
    }
}

void FontConvertor::SetDefaultParams()
{
    params = Params::GetDefault();
}

bool FontConvertor::Convert()
{
    assert(font);
    if (!font->Init(params.filename))
    {
        return false;
    }

    font->SetCharMap(params.charmap);
    if (!FillCharList())
    {
        return false;
    }

    bool ok = true;
    switch (params.mode)
    {
    case MODE_ADJUST_FONT:
        ok = AdjustFontSize(params.fontSize);
        break;

    case MODE_ADJUST_TEXTURE:
        ok = AdjustTextureSize(params.textureSize);
        break;

    case MODE_GENERATE:
        break;

    default:
        break;
    }

    if (!ok)
    {
        cerr << endl
             << "Error: chars will not fit into texture" << endl;
        return false;
    }

    cout << "Packing chars into texture...";
    if (!GeneratePackedList(params.fontSize, params.textureSize))
    {
        cerr << endl
             << "Error: chars will not fit into texture" << endl;
        return false;
    }
    cout << " Done" << endl;

    FillKerning();
    GenerateOutputImage();
    GenerateFontDescription();

    return true;
}

void FontConvertor::GenerateFontDescription()
{
    cout << "Storing font description...";

    auto oldSize = font->GetSize();
    font->SetSize(params.fontSize);
    auto lineHeight = static_cast<uint32>(ceil(font->GetLineHeight()));
    auto baselineHeight = static_cast<uint32>(ceil(font->GetBaseline()));
    font->SetSize(oldSize);

    ofstream outFile;
    outFile.open(params.filename + ".fnt");

    outFile << "font:" << endl;
    outFile << "  name: "
            << "" << endl;
    outFile << "  size: " << params.fontSize << endl;
    outFile << "  lineHeight: " << lineHeight << endl;
    outFile << "  baselineHeight: " << baselineHeight << endl;
    outFile << "  scaleW: " << params.textureSize << endl;
    outFile << "  scaleH: " << params.textureSize << endl;

    outFile << "  distanceFieldFont: " << (params.output == TYPE_DISTANCE_FIELD ? "true" : "false") << endl;
    outFile << "  spread: " << params.spread << endl;

    outFile << "  chars: " << endl;
    auto it = chars.begin();
    const auto itEnd = chars.end();
    for (; it != itEnd; ++it)
    {
        auto& desc = it->second;

        auto u = static_cast<float32>(desc.x) / params.textureSize;
        auto v = static_cast<float32>(desc.y) / params.textureSize;
        auto u2 = static_cast<float32>(desc.x + desc.width) / params.textureSize;
        auto v2 = static_cast<float32>(desc.y + desc.height) / params.textureSize;

        outFile << "    " << desc.id << ": {xoffset: " << desc.xOffset << ", yoffset: " << desc.yOffset;
        outFile << ", width: " << desc.width << ", height: " << desc.height << ", xadvance: " << desc.xAdvance;
        outFile << ", u: " << u << ", v: " << v << ", u2: " << u2 << ", v2: " << v2 << "}" << endl;
    }

    if (kerningCount)
    {
        outFile << "  kerning:" << endl;

        for (it = chars.begin(); it != itEnd; ++it)
        {
            auto& desc = it->second;

            ostringstream os;
            auto cnt = 0;
            auto kernIt = desc.kernings.begin();
            const auto kernItEnd = desc.kernings.end();
            for (; kernIt != kernItEnd; ++kernIt)
            {
                if (cnt++)
                {
                    os << ", ";
                }

                os << kernIt->first << ": " << kernIt->second;
            }

            if (cnt)
            {
                outFile << "    " << desc.id << ": {" << os.str() << "}" << endl;
            }
        }
    }

    outFile.close();

    cout << "Done" << endl
         << endl;
}

void FontConvertor::FillKerning()
{
    cout << "Preparing kerning information..." << endl;

    auto oldSize = font->GetSize();
    font->SetSize(params.fontSize * params.scale);
    auto& face = font->GetFace();
    kerningCount = 0;

    int percDone = 0;
    int cnt = 0;
    int size = (int)chars.size();

    const auto itEnd = chars.end();
    for (auto i = chars.begin(); i != itEnd; ++i)
    {
        auto charIndexLeft = FT_Get_Char_Index(face, i->second.id);

        for (auto j = chars.begin(); j != itEnd; ++j)
        {
            auto charIndexRight = FT_Get_Char_Index(face, j->second.id);

            FT_Vector kerning;
            if (!FT_Get_Kerning(face, charIndexLeft, charIndexRight, FT_KERNING_DEFAULT, &kerning))
            {
                if (kerning.x)
                {
                    auto scaledKern = static_cast<float32>(kerning.x) / 64.f / params.scale;
                    i->second.kernings[j->second.id] = scaledKern;
                    ++kerningCount;
                }
            }
        }

        // Calculate progress
        cnt++;
        auto p = static_cast<int32>(floor((static_cast<float32>(cnt) / size) * 100.f));
        if (p > percDone)
        {
            percDone = p;
            cout << setw(3) << p << "% done" << endl;
        }
    }

    font->SetSize(oldSize);

    cout << kerningCount << " kerning pairs found" << endl
         << endl;
}

bool FontConvertor::FillCharList()
{
    cout << "Preparing char list...";

    Vector<int32> unfilteredChars;

    if (params.charListFile != "")
    {
        LoadCharList(unfilteredChars);
    }
    else
    {
        for (auto i = 0; i <= params.maxChar; ++i)
        {
            unfilteredChars.push_back(i);
        }
    }

    auto& face = font->GetFace();

    auto size = unfilteredChars.size();
    for (auto i = 0U; i < unfilteredChars.size(); ++i)
    {
        auto glyphIndex = FT_Get_Char_Index(face, unfilteredChars[i]); // Get glyph index
        if (glyphIndex)
        {
            auto err = FT_Load_Glyph(face, glyphIndex, 0); // Check that glyph exist
            if (!err)
            {
                err = FT_Render_Glyph(face->glyph, FT_RENDER_MODE_MONO); // Check that glyph draw
                if (!err)
                {
                    charGlyphPairs.push_back(make_pair(unfilteredChars[i], glyphIndex));
                }
            }
        }
    }

    // Additionally add space as NOT_DEF_CHAR
    {
        auto glyphIndex = FT_Get_Char_Index(face, NOT_DEF_REPLACEMENT_ASCII_CODE); // Space character
        if (!glyphIndex)
        {
            cerr << "Font doesn't contain space character. This font could not be converted." << endl;
            return false;
        }
        charGlyphPairs.push_back(make_pair(NOT_DEF_CHAR, glyphIndex));
    }

    cout << " " << charGlyphPairs.size() << " characters found" << endl
         << endl;
    return true;
}

void FontConvertor::LoadCharList(Vector<int32>& charList)
{
    Vector<String> strings;

    ifstream inFile(params.charListFile);
    if (inFile.is_open())
    {
        string line;
        while (getline(inFile, line))
        {
            strings.push_back(line);
        }

        inFile.close();
    }

    auto size = strings.size();
    for (auto i = 0U; i < size; ++i)
    {
        WideString ws = UTF8Utils::EncodeToWideString(strings[i]);
        charList.insert(charList.end(), ws.begin(), ws.end());
    }

    // Sort chars
    sort(charList.begin(), charList.end());
    // Delete duplicates
    charList.erase(unique(charList.begin(), charList.end()), charList.end());
}

bool FontConvertor::AdjustFontSize(int32& size)
{
    cout << "Adjusting font size..." << endl;
    auto curSize = Params::GetDefault().fontSize >> 1;

    auto keepGoing = true;
    while (keepGoing)
    {
        curSize <<= 1;

        cout << curSize << " ";
        keepGoing = GeneratePackedList(curSize, params.textureSize);
    }

    auto step = curSize >> 2;
    while (step)
    {
        curSize += (keepGoing ? 1 : -1) * step;
        step >>= 1;

        cout << curSize << " ";
        keepGoing = GeneratePackedList(curSize, params.textureSize);
    }

    while (!keepGoing && curSize > 1)
    {
        --curSize;

        cout << curSize << " ";
        keepGoing = GeneratePackedList(curSize, params.textureSize);
    }

    cout << endl
         << "Done" << endl;
    cout << endl
         << "Font size: " << curSize << endl;

    if (!keepGoing)
    {
        return false;
    }

    size = curSize;

    return true;
}

bool FontConvertor::AdjustTextureSize(int32& size)
{
    cout << "Adjusting texture size..." << endl;

    auto curSize = Params::GetDefault().textureSize >> 1;

    auto keepGoing = false;
    while (!keepGoing && curSize <= MAX_TEXTURE_SIZE)
    {
        curSize <<= 1;

        cout << curSize << " ";
        keepGoing = GeneratePackedList(params.fontSize, curSize);
    }

    if (!keepGoing)
    {
        return false;
    }

    while (keepGoing && curSize >= MIN_TEXTURE_SIZE)
    {
        curSize >>= 1;

        cout << curSize << " ";
        keepGoing = GeneratePackedList(params.fontSize, curSize);
    }

    curSize <<= 1;
    if (curSize < MIN_TEXTURE_SIZE)
    {
        curSize = MIN_TEXTURE_SIZE;
    }

    cout << endl
         << "Done" << endl;
    cout << "Texture size: " << curSize << endl;

    size = curSize;

    return true;
}

void FontConvertor::GenerateOutputImage()
{
    cout << "Converting..." << endl;

    auto oldSize = font->GetSize();
    font->SetSize(params.fontSize * params.scale);
    auto& face = font->GetFace();

    Vector<uint8> imgData(4 * params.textureSize * params.textureSize, 0);

    auto percentDone = 0;

    int i = 0;
    auto iter = charGlyphPairs.begin();
    const auto iterEnd = charGlyphPairs.end();
    for (; iter != iterEnd; ++iter)
    {
        int glyphIndex = iter->second;

        auto err = FT_Load_Glyph(face, glyphIndex, 0);
        if (!err)
        {
            uint8* charBuf = nullptr;
            auto charWidth = 0;
            auto charHeight = 0;

            if (iter->first == NOT_DEF_CHAR) // Draw NOT_DEF_CHAR
            {
                auto leading = static_cast<float32>(face->size->metrics.height + face->size->metrics.descender) / 64.f;
                auto glyphWidth = static_cast<int32>(face->glyph->advance.x) / 64;
                auto glyphHeight = static_cast<int32>(leading * 0.75f);

                //	oversize the holding buffer by spread value to be filled in with distance blur
                charWidth = glyphWidth + params.scale * (params.spread * 2);
                charHeight = glyphHeight + params.scale * (params.spread * 2);

                charBuf = new uint8[charWidth * charHeight];
                memset(charBuf, 0, sizeof(uint8) * charWidth * charHeight);

                // Draw NOT_DEF_CHAR as rect
                auto lineWidth = min(charWidth, charHeight);
                lineWidth = max(1, lineWidth / 8);
                for (auto j = 0; j < glyphHeight; ++j)
                {
                    for (auto i = 0; i < lineWidth; ++i)
                    {
                        auto y = (j + params.scale * params.spread) * charWidth;
                        auto x = i + params.scale * params.spread;

                        charBuf[y + x] = 255;
                        charBuf[y + (charWidth - 1) - x] = 255;
                    }
                }
                auto maxY = (charHeight - 1) * charWidth;
                for (auto i = 0; i < glyphWidth; ++i)
                {
                    for (auto j = 0; j < lineWidth; ++j)
                    {
                        auto y = (j + params.scale * params.spread) * charWidth;
                        auto x = i + params.scale * params.spread;

                        charBuf[y + x] = 255;
                        charBuf[maxY - y + x] = 255;
                    }
                }
            }
            else
            {
                err = FT_Render_Glyph(face->glyph, params.output == TYPE_DISTANCE_FIELD ? FT_RENDER_MODE_MONO : FT_RENDER_MODE_NORMAL);
                if (!err)
                {
                    auto glyphWidth = face->glyph->bitmap.width;
                    auto glyphHeight = face->glyph->bitmap.rows;

                    // Oversize the holding buffer by spread value to be filled in with distance blur
                    charWidth = glyphWidth + params.scale * (params.spread * 2);
                    charHeight = glyphHeight + params.scale * (params.spread * 2);

                    charBuf = new uint8[charWidth * charHeight];
                    memset(charBuf, 0, sizeof(uint8) * charWidth * charHeight);

                    // Copy the glyph into the buffer to be smoothed
                    auto glyphPitch = face->glyph->bitmap.pitch;
                    auto& glyphBuf = face->glyph->bitmap.buffer;

                    if (params.output == TYPE_DISTANCE_FIELD)
                    {
                        for (auto j = 0U; j < glyphHeight; ++j)
                        {
                            for (auto i = 0U; i < glyphWidth; ++i)
                            {
                                //check if corresponding bit is set
                                auto glyphVal = glyphBuf[j * glyphPitch + (i >> 3)];
                                auto val = (glyphVal >> (7 - (i & 7))) & 1;

                                auto x = i + params.scale * params.spread;
                                auto y = (j + params.scale * params.spread) * charWidth;
                                charBuf[y + x] = 255 * val;
                            }
                        }
                    }
                    else //params.output == TYPE_GRAPHIC
                    {
                        for (auto j = 0U; j < glyphHeight; ++j)
                        {
                            for (auto i = 0U; i < glyphWidth; ++i)
                            {
                                auto x = i + params.scale * params.spread;
                                auto y = (j + params.scale * params.spread) * charWidth;
                                charBuf[y + x] = glyphBuf[j * glyphPitch + i];
                            }
                        }
                    }
                }
            }

            auto& desc = chars[iter->first];
            if (params.output == TYPE_DISTANCE_FIELD)
            {
                auto* distanceBuf = BuildDistanceField(charBuf, charWidth, charHeight);
                for (auto i = 0; i < desc.height; ++i)
                {
                    auto offset = ((desc.y + i) * params.textureSize + desc.x) * 4;
                    for (auto j = 0; j < desc.width; ++j)
                    {
                        imgData[offset + 0] = 0xff;
                        imgData[offset + 1] = 0xff;
                        imgData[offset + 2] = 0xff;
                        imgData[offset + 3] = distanceBuf[i * desc.width + j];
                        offset += 4;
                    }
                }
                delete[] distanceBuf;
            }
            else //params.output == TYPE_GRAPHIC
            {
                for (auto i = 0; i < desc.height; ++i)
                {
                    auto offset = ((desc.y + i) * params.textureSize + desc.x) * 4;
                    for (auto j = 0; j < desc.width; ++j)
                    {
                        imgData[offset + 0] = 0xff;
                        imgData[offset + 1] = 0xff;
                        imgData[offset + 2] = 0xff;
                        imgData[offset + 3] = charBuf[i * desc.width + j];
                        offset += 4;
                    }
                }
            }

            delete[] charBuf;
        }

        // Calculate process
        auto p = static_cast<int32>(floor((static_cast<float32>(i++) / charGlyphPairs.size()) * 100.f));
        if (p > percentDone)
        {
            cout << setw(3) << p << "% converted" << endl;
            percentDone = p;
        }
    }

    font->SetSize(oldSize);
    cout << " Done" << endl;

    cout << "Storing image...";
    Vector<uint8> buffer;
    LodePNG::Encoder encoder;
    encoder.getSettings().zlibsettings.windowSize = 512;
    encoder.encode(buffer, imgData, params.textureSize, params.textureSize);
    LodePNG::saveFile(buffer, params.filename + ".png");
    cout << " Done" << endl;
}

uint8* FontConvertor::BuildDistanceField(const uint8* inputBuf, int32 inWidth, int32 inHeight)
{
    auto outWidth = inWidth / params.scale;
    auto outHeight = inHeight / params.scale;

    auto* outBuf = new uint8[outWidth * outHeight];

    for (auto y = 0; y < outHeight; ++y)
    {
        for (auto x = 0; x < outWidth; ++x)
        {
            auto centerX = x * params.scale + params.scale / 2;
            auto centerY = y * params.scale + params.scale / 2;

            auto dist = FindSignedDistance(centerX, centerY, inputBuf, inWidth, inHeight);
            outBuf[y * outWidth + x] = DistanceToAlpha(dist);
        }
    }

    return outBuf;
}

uint8 FontConvertor::DistanceToAlpha(float32 distance)
{
    auto alpha = 0.5f + 0.5f * (distance / (params.spread * params.scale));
    alpha = min(1.f, max(0.f, alpha));
    return static_cast<uint8>(alpha * 0xff);
}

float32 FontConvertor::FindSignedDistance(int32 centerX, int32 centerY, const uint8* inputBuf, int32 width, int32 height)
{
    auto baseVal = inputBuf[centerY * width + centerX] != 0;
    auto scaledSpread = params.spread * params.scale;

    auto startX = max(0, centerX - scaledSpread);
    auto endX = min(width - 1, centerX + scaledSpread);
    auto startY = max(0, centerY - scaledSpread);
    auto endY = min(height - 1, centerY + scaledSpread);

    auto minSquareDist = scaledSpread * scaledSpread;

    for (auto y = startY; y <= endY; ++y)
    {
        for (auto x = startX; x <= endX; ++x)
        {
            auto curVal = inputBuf[y * width + x] != 0;
            if (baseVal != curVal)
            {
                auto dx = centerX - x;
                auto dy = centerY - y;
                auto squareDist = dx * dx + dy * dy;

                if (squareDist < minSquareDist)
                {
                    minSquareDist = squareDist;
                }
            }
        }
    }

    auto minDist = sqrtf(static_cast<float32>(minSquareDist));
    return (baseVal ? 1.f : -1.f) * min(minDist, static_cast<float32>(scaledSpread));
}

bool FontConvertor::GeneratePackedList(int32 fontSize, int32 textureSize)
{
    chars.clear();

    Vector<int32> rectInfo;
    auto renderMode = params.output == TYPE_DISTANCE_FIELD ? FT_RENDER_MODE_MONO : FT_RENDER_MODE_NORMAL;

    auto oldSize = font->GetSize();
    font->SetSize(fontSize * params.scale);
    auto& face = font->GetFace();

    // Here we should determine max glyph height, to get proper yOffset later.
    auto maxBitmapTop = 0;
    auto iter = charGlyphPairs.begin();
    auto iterEnd = charGlyphPairs.end();
    for (; iter != iterEnd; ++iter)
    {
        auto glyphIndex = iter->second;
        auto err = FT_Load_Glyph(face, glyphIndex, 0);
        if (!err)
        {
            err = FT_Render_Glyph(face->glyph, renderMode);
            if (!err)
            {
                if (maxBitmapTop < face->glyph->bitmap_top)
                {
                    maxBitmapTop = face->glyph->bitmap_top;
                }
            }
        }
    }

    iter = charGlyphPairs.begin();
    for (; iter != iterEnd; ++iter)
    {
        auto glyphIndex = iter->second; // We use for NOT_DEF_CHAR space code (see FillCharList)
        auto err = FT_Load_Glyph(face, glyphIndex, 0);
        if (!err)
        {
            err = FT_Render_Glyph(face->glyph, renderMode);
            if (!err)
            {
                FontConvertor::CharDescription charDesc;
                charDesc.x = -1;
                charDesc.y = -1;
                charDesc.id = iter->first;
                charDesc.xOffset = static_cast<float32>(face->glyph->bitmap_left);
                charDesc.xAdvance = static_cast<float32>(face->glyph->advance.x) / 64.f;

                if (iter->first == NOT_DEF_CHAR)
                {
                    auto leading = static_cast<float32>(face->size->metrics.height + face->size->metrics.descender) / 64.f;
                    charDesc.width = static_cast<int32>(face->glyph->advance.x) / 64;
                    charDesc.height = static_cast<int32>(leading * 0.75f);
                    charDesc.yOffset = leading * 0.25f;
                }
                else
                {
                    charDesc.width = face->glyph->bitmap.width;
                    charDesc.height = face->glyph->bitmap.rows;
                    charDesc.yOffset = static_cast<float32>(maxBitmapTop - face->glyph->bitmap_top);
                }

                // Oversize the holding buffer by spread value to be filled in with distance blur
                if (charDesc.width > 0)
                {
                    charDesc.width += params.spread * 2 * params.scale;
                }
                if (charDesc.height > 0)
                {
                    charDesc.height += params.spread * 2 * params.scale;
                }

                // Normalize char metrics
                charDesc.width /= params.scale;
                charDesc.height /= params.scale;
                charDesc.yOffset /= params.scale;
                charDesc.xOffset /= params.scale;
                charDesc.xAdvance /= params.scale;

                chars[charDesc.id] = charDesc;

                rectInfo.push_back(charDesc.width);
                rectInfo.push_back(charDesc.height);
            }
        }
    }

    font->SetSize(oldSize);

    BinPacker bp;
    Vector<Vector<int32>> packedInfo;
    bp.Pack(rectInfo, packedInfo, textureSize, false);

    if (packedInfo.size() == 1)
    {
        auto cnt = packedInfo[0].size();
        for (auto i = 0U; i < cnt; i += 4)
        {
            //	index, x, y, rotated
            auto index = charGlyphPairs[packedInfo[0][i]].first;
            chars[index].x = packedInfo[0][i + 1];
            chars[index].y = packedInfo[0][i + 2];
        }
        return true;
    }

    return false;
}

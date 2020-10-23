#pragma once

#include "Render/PixelFormatDescriptor.h"
#include "Math/HalfFloat.h"

namespace DAVA
{
#pragma pack(push, 1)

#define RGB_PIXEL(TYPE, C1, C2, C3) struct { TYPE C1, C2, C3; };
#define RGBA_PIXEL(TYPE, C1, C2, C3, C4) struct { TYPE C1, C2, C3, C4; };

using RGB888 = RGB_PIXEL(uint8, r, g, b);
using BGR888 = RGB_PIXEL(uint8, b, g, r);
using BGRA8888 = RGBA_PIXEL(uint8, b, g, r, a);
using RGBA8888 = RGBA_PIXEL(uint8, r, g, b, a);
using ARGB8888 = RGBA_PIXEL(uint8, a, r, g, b);
using ABGR8888 = RGBA_PIXEL(uint8, a, b, g, r);
using RGBA16161616 = RGBA_PIXEL(uint16, r, g, b, a);
using ABGR16161616 = RGBA_PIXEL(uint16, a, b, g, r);
using RGBA32323232 = RGBA_PIXEL(uint32, r, g, b, a);
using ABGR32323232 = RGBA_PIXEL(uint32, a, b, g, r);
using RGB16F = RGB_PIXEL(uint16, r, g, b);
using RGB32F = RGB_PIXEL(float32, r, g, b);
using RGBA16F = RGBA_PIXEL(uint16, r, g, b, a);
using ABGR16F = RGBA_PIXEL(uint16, a, b, g, r);
using RGBA32F = RGBA_PIXEL(float32, r, g, b, a);
using ABGR32F = RGBA_PIXEL(float32, a, b, g, r);

#pragma pack(pop)

struct ConvertBGRA8888toRGBA8888
{
    inline void operator()(const BGRA8888* input, RGBA8888* output)
    {
        RGBA8888 tmp = { input->r, input->g, input->b, input->a };
        *output = tmp;
    }
};

struct ConvertRGBA8888toBGRA8888
{
    inline void operator()(const RGBA8888* input, BGRA8888* output)
    {
        BGRA8888 tmp = { input->r, input->g, input->b, input->a };
        *output = tmp;
    }
};

struct ConvertARGB8888toRGBA8888
{
    inline void operator()(const ARGB8888* input, RGBA8888* output)
    {
        const ARGB8888 inp = *input;
        output->r = inp.r;
        output->g = inp.g;
        output->b = inp.b;
        output->a = inp.a;
    }
};

struct ConvertABGR8888toRGBA8888
{
    inline void operator()(const ABGR8888* input, RGBA8888* output)
    {
        const ABGR8888 inp = *input;
        output->r = inp.r;
        output->g = inp.g;
        output->b = inp.b;
        output->a = inp.a;
    }
};

struct ConvertARGB1555toRGBA5551
{
    inline void operator()(const uint16* input, uint16* output)
    {
        //arrr rrgg gggb bbbb --> rrrr rggg ggbb bbba
        const uint16 in = *input;
        uint16 r = (in & 0x003e) << 9;
        uint16 g = (in & 0x07c0) >> 1;
        uint16 b = (in & 0xf800) >> 11;
        uint16 a = (in & 0x0001) << 15;

        *output = r | g | b | a;
    }
};

struct ConvertRGBA8888toRGB888
{
    inline void operator()(const uint32* input, RGB888* output)
    {
        uint32 pixel = *input;
        output->b = ((pixel >> 16) & 0xFF);
        output->g = ((pixel >> 8) & 0xFF);
        output->r = (pixel & 0xFF);
    }
};

struct ConvertBGRA8888toRGB888
{
    inline void operator()(const BGRA8888* input, RGB888* output)
    {
        RGB888 tmp{ input->r, input->g, input->b };
        *output = tmp;
    }
};

struct ConvertRGB888toRGBA8888
{
    inline void operator()(const RGB888* input, uint32* output)
    {
        *output = ((0xFF) << 24) | (input->b << 16) | (input->g << 8) | input->r;
    }
};

struct ConvertRGB888toBGRA8888
{
    inline void operator()(const RGB888* input, BGRA8888* output)
    {
        BGRA8888 tmp{ input->b, input->g, input->r, 0xFF };
        *output = tmp;
    }
};

struct ConvertBGR888toRGBA8888
{
    inline void operator()(const BGR888* input, uint32* output)
    {
        *output = ((0xFF) << 24) | (input->b << 16) | (input->g << 8) | input->r;
    }
};

struct ConvertRGBA16161616toRGBA8888
{
    inline void operator()(const RGBA16161616* input, uint32* output)
    {
        uint8 r = (input->r >> 8) & 0xFF;
        uint8 g = (input->g >> 8) & 0xFF;
        uint8 b = (input->b >> 8) & 0xFF;
        uint8 a = (input->a >> 8) & 0xFF;
        *output = (a << 24) | (b << 16) | (g << 8) | r;
    }
};

struct ConvertRGBA32323232toRGBA8888
{
    inline void operator()(const RGBA32323232* input, uint32* output)
    {
        uint8 r = (input->r >> 24) & 0xFF;
        uint8 g = (input->g >> 24) & 0xFF;
        uint8 b = (input->b >> 24) & 0xFF;
        uint8 a = (input->a >> 24) & 0xFF;
        *output = (a << 24) | (b << 16) | (g << 8) | r;
    }
};

struct ConvertABGR16161616toRGBA16161616
{
    inline void operator()(const ABGR16161616* input, RGBA16161616* output)
    {
        ABGR16161616 inp = *input;
        output->r = inp.r;
        output->g = inp.g;
        output->b = inp.b;
        output->a = inp.a;
    }
};

struct ConvertABGR16FtoRGBA16F
{
    inline void operator()(const ABGR16F* input, RGBA16F* output)
    {
        ABGR16F inp = *input;
        output->r = inp.r;
        output->g = inp.g;
        output->b = inp.b;
        output->a = inp.a;
    }
};

struct ConvertABGR32323232toRGBA32323232
{
    inline void operator()(const ABGR32323232* input, RGBA32323232* output)
    {
        ABGR32323232 inp = *input;
        output->r = inp.r;
        output->g = inp.g;
        output->b = inp.b;
        output->a = inp.a;
    }
};

struct ConvertABGR32FtoRGBA32F
{
    inline void operator()(const ABGR32F* input, RGBA32F* output)
    {
        ABGR32F inp = *input;
        output->r = inp.r;
        output->g = inp.g;
        output->b = inp.b;
        output->a = inp.a;
    }
};

struct ConvertBGR888toRGB888
{
    inline void operator()(const BGR888* input, RGB888* output)
    {
        BGR888 tmp = *input;
        output->r = tmp.r;
        output->g = tmp.g;
        output->b = tmp.b;
    }
};

struct ConvertA16toA8
{
    inline void operator()(const uint16* input, uint8* output)
    {
        uint16 pixel = *input;
        *output = uint8(pixel);
    }
};

struct ConvertA8toA16
{
    inline void operator()(const uint8* input, uint16* output)
    {
        uint8 pixel = *input;
        *output = pixel;
    }
};

struct ConvertRGBA8888toRGBA4444
{
    inline void operator()(const uint32* input, uint16* output)
    {
        uint32 pixel = *input;
        uint32 a = ((pixel >> 24) & 0xFF) >> 4;
        uint32 b = ((pixel >> 16) & 0xFF) >> 4;
        uint32 g = ((pixel >> 8) & 0xFF) >> 4;
        uint32 r = (pixel & 0xFF) >> 4;
        *output = ((r) << 12) | (g << 8) | (b << 4) | a;
    }
};

struct ConvertRGBA5551toRGBA8888
{
    inline void operator()(const uint16* input, uint32* output)
    {
        uint16 pixel = *input;

        uint32 a = ((pixel >> 15) & 0x01) ? 0x00FF : 0;
        uint32 b = (((pixel >> 10) & 0x01F) << 3);
        uint32 g = (((pixel >> 5) & 0x01F) << 3);
        uint32 r = (((pixel >> 0) & 0x01F) << 3);

        *output = (r) | (g << 8) | (b << 16) | (a << 24);
    }
};

struct ConvertRGBA4444toRGBA8888
{
    inline void operator()(const uint16* input, uint32* output)
    {
        uint16 pixel = *input;
        uint32 a = (((pixel >> 12) & 0x0F) << 4);
        uint32 b = (((pixel >> 8) & 0x0F) << 4);
        uint32 g = (((pixel >> 4) & 0x0F) << 4);
        uint32 r = (((pixel >> 0) & 0x0F) << 4);

        *output = (r) | (g << 8) | (b << 16) | (a << 24);
    }
};

struct ConvertRGB565toRGBA8888
{
    inline void operator()(const uint16* input, uint32* output)
    {
        //r-channel in least significant bits
        uint16 pixel = *input;
        uint32 b = (((pixel >> 11) & 0x01F) << 3);
        uint32 g = (((pixel >> 5) & 0x03F) << 2);
        uint32 r = (((pixel >> 0) & 0x01F) << 3);
        uint32 a = 0xFF;

        *output = (r) | (g << 8) | (b << 16) | (a << 24);
    }
};

struct ConvertA8toRGBA8888
{
    inline void operator()(const uint8* input, uint32* output)
    {
        uint32 pixel = *input;
        *output = (0xFF << 24) | (pixel << 16) | (pixel << 8) | pixel;
    }
};

struct ConvertA16toRGBA8888
{
    inline void operator()(const uint16* input, uint32* output)
    {
        uint32 pixel = *input;
        *output = (0xFF << 24) | (pixel << 16) | (pixel << 8) | pixel;
    }
};

struct ConvertBGRA4444toRGBA4444
{
    inline void operator()(const uint16* input, uint16* output)
    {
        //bbbb gggg rrrr aaaa --> rrrr gggg bbbb aaaa
        const uint16 in = *input;
        uint16 greenAlpha = in & 0x0F0F;
        uint16 blue = (in >> 8) & 0x00F0;
        uint16 red = (in & 0x00F0) << 8;
        *output = red | greenAlpha | blue;
    }
};

struct ConvertABGR4444toRGBA4444
{
    inline void operator()(const uint16* input, uint16* output)
    {
        const uint8* in = reinterpret_cast<const uint8*>(input);
        uint8* out = reinterpret_cast<uint8*>(output);

        //aaaa bbbb gggg rrrr --> rrrr gggg bbbb aaaa
        uint8 ab = in[0];
        uint8 gr = in[1];

        out[0] = ((gr & 0x0f) << 4) | ((gr & 0xf0) >> 4); //rg
        out[1] = ((ab & 0x0f) << 4) | ((ab & 0xf0) >> 4); //ba
    }
};

struct ConvertRGBA4444toABGR4444
{
    inline void operator()(const uint16* input, uint16* output)
    {
        const uint8* in = reinterpret_cast<const uint8*>(input);
        uint8* out = reinterpret_cast<uint8*>(output);

        //rrrr gggg bbbb aaaa --> aaaa bbbb gggg rrrr
        uint8 rg = in[0];
        uint8 ba = in[1];

        out[0] = ((ba & 0x0f) << 4) | ((ba & 0xf0) >> 4); //ab
        out[1] = ((rg & 0x0f) << 4) | ((rg & 0xf0) >> 4); //gr
    }
};

struct ConvertARGB4444toRGBA4444
{
    inline void operator()(const uint16* input, uint16* output)
    {
        //aaaa rrrr gggg bbbb --> rrrr gggg bbbb aaaa
        uint16 a = *input & 0xF;
        *output = (*input >> 4) | (a << 12);
    }
};

struct ConvertRGBA4444toARGB4444
{
    inline void operator()(const uint16* input, uint16* output)
    {
        // src: aaaa bbbb gggg rrrr
        // dst: bbbb gggg rrrr aaaa
        uint16 a = *input >> 12;
        *output = (*input << 4) | a;
    }
};

struct ConvertBGRA5551toRGBA5551
{
    inline void operator()(const uint16* input, uint16* output)
    {
        //bbbb bggg ggrr rrra --> rrrr rggg ggbb bbba
        const uint16 in = *input;
        uint16 r = (in & 0x7c00) >> 5;
        uint16 b = (in & 0x001f) << 5;
        uint16 ga = in & 0x83e0;

        *output = r | b | ga;
    }
};

struct ConvertABGR1555toRGBA5551
{
    inline void operator()(const uint16* input, uint16* output)
    {
        //abbb bbgg gggr rrrr --> rrrr rggg ggbb bbba
        const uint16 in = *input;
        uint16 r = (in & 0xf800) >> 11;
        uint16 g = (in & 0x07c0) >> 1;
        uint16 b = (in & 0x003e) << 9;
        uint16 a = (in & 0x0001) << 15;

        *output = r | g | b | a;
    }
};

struct ConvertRGBA5551toABGR1555
{
    inline void operator()(const uint16* input, uint16* output)
    { //-----based on ConvertABGR1555toRGBA5551
        //rrrr rggg ggbb bbba --> abbb bbgg gggr rrrr
        const uint16 in = *input;

        uint16 a = (in >> 15) & 0x01;
        uint16 b = (in >> 10) & 0x1F;
        uint16 g = (in >> 5) & 0x1F;
        uint16 r = (in >> 0) & 0x1F;

        *output = a | (b << 1) | (g << 6) | (r << 11);
    }
};

struct ConvertRGBA5551toARGB1555
{
    inline void operator()(const uint16* input, uint16* output)
    {
        // inp: abbb bbgg gggr rrrr
        // out: bbbb bggg ggrr rrra
        uint16 a = *input >> 15;
        *output = (*input << 1) | a;
    }
};

struct ConvertBGR565toRGB565
{
    inline void operator()(const uint16* input, uint16* output)
    {
        // bbbb bggg gggr rrrr --> rrrr rggg gggb bbbb
        uint16 in = *input;
        uint16 blue = (in >> 11) & 0x1F;
        uint16 red = (in & 0x1F) << 11;
        uint16 green = in & 0x07E0;
        *output = red | green | blue;
    }
};

struct ConvertBGRA16161616toRGBA16161616
{
    inline void operator()(const RGBA16161616* input, RGBA16161616* output)
    {
        RGBA16161616 out = *input;
        uint16 tmp = out.r;
        out.r = out.b;
        out.b = tmp;
        *output = out;
    }
};

struct ConvertBGRA32323232toRGBA32323232
{
    inline void operator()(const RGBA32323232* input, RGBA32323232* output)
    {
        RGBA32323232 out = *input;
        uint32 tmp = out.r;
        out.r = out.b;
        out.b = tmp;
        *output = out;
    }
};

uint32 ChannelFloatToInt(float32 ch);
float32 ChannelIntToFloat(uint32 ch);

struct ConvertRGBA32FtoRGBA8888
{
    inline void operator()(const RGBA32F* input, uint32* output)
    {
        RGBA32F inp = *input;
        uint32 r = ChannelFloatToInt(inp.r);
        uint32 g = ChannelFloatToInt(inp.g);
        uint32 b = ChannelFloatToInt(inp.b);
        uint32 a = ChannelFloatToInt(inp.a);
        *output = (r) | (g << 8) | (b << 16) | (a << 24);
    }
};

struct ConvertRGBA8888toRGBA32F
{
    inline void operator()(const uint32* input, RGBA32F* output)
    {
        uint32 pixel = *input;
        uint32 a = (pixel >> 24) & 0xFF;
        uint32 b = (pixel >> 16) & 0xFF;
        uint32 g = (pixel >> 8) & 0xFF;
        uint32 r = (pixel & 0xFF);

        output->r = ChannelIntToFloat(r);
        output->g = ChannelIntToFloat(g);
        output->b = ChannelIntToFloat(b);
        output->a = ChannelIntToFloat(a);
    }
};

struct ConvertRGBA16FtoRGBA8888
{
    inline void operator()(const RGBA16F* input, uint32* output)
    {
        RGBA32F input32;
        input32.r = Float16Compressor::Decompress(input->r);
        input32.g = Float16Compressor::Decompress(input->g);
        input32.b = Float16Compressor::Decompress(input->b);
        input32.a = Float16Compressor::Decompress(input->a);
        ConvertRGBA32FtoRGBA8888 convert32;
        convert32(&input32, output);
    }
};

struct ConvertRGBA8888toRGBA16F
{
    inline void operator()(const uint32* input, RGBA16F* output)
    {
        uint32 pixel = *input;
        uint32 a = (pixel >> 24) & 0xFF;
        uint32 b = (pixel >> 16) & 0xFF;
        uint32 g = (pixel >> 8) & 0xFF;
        uint32 r = (pixel & 0xFF);

        output->r = Float16Compressor::Compress(ChannelIntToFloat(r));
        output->g = Float16Compressor::Compress(ChannelIntToFloat(g));
        output->b = Float16Compressor::Compress(ChannelIntToFloat(b));
        output->a = Float16Compressor::Compress(ChannelIntToFloat(a));
    }
};

struct UnpackRGBA8888
{
    inline void operator()(const uint32* input, uint32& r, uint32& g, uint32& b, uint32& a)
    {
        uint32 pixel = *input;
        a = ((pixel >> 24) & 0xFF);
        r = ((pixel >> 16) & 0xFF);
        g = ((pixel >> 8) & 0xFF);
        b = (pixel & 0xFF);
    }
};

struct PackRGBA8888
{
    inline void operator()(uint32 r, uint32 g, uint32 b, uint32 a, uint32* output)
    {
        *output = ((a) << 24) | (r << 16) | (g << 8) | b;
    }
};

struct UnpackRGBA4444
{
    inline void operator()(const uint16* input, uint32& r, uint32& g, uint32& b, uint32& a)
    {
        uint16 pixel = *input;
        a = ((pixel >> 12) & 0xF);
        r = ((pixel >> 8) & 0xF);
        g = ((pixel >> 4) & 0xF);
        b = (pixel & 0xF);
    }
};

struct PackRGBA4444
{
    inline void operator()(uint32 r, uint32 g, uint32 b, uint32 a, uint16* output)
    {
        *output = ((b >> 4) << 12) | ((g >> 4) << 8) | ((r >> 4) << 4) | (a >> 4);
    }
};

struct UnpackA8
{
    inline void operator()(const uint8* input, uint32& r, uint32& g, uint32& b, uint32& a)
    {
        r = g = b = 0;
        a = (*input);
    }
};

struct PackA8
{
    inline void operator()(uint32 r, uint32 g, uint32 b, uint32 a, uint8* output)
    {
        *output = a;
    }
};

struct UnpackRGB888
{
    inline void operator()(const RGB888* input, uint32& r, uint32& g, uint32& b, uint32& a)
    {
        r = input->r;
        g = input->g;
        b = input->b;
        a = 0xFF;
    }
};

struct PackRGB888
{
    inline void operator()(uint32 r, uint32 g, uint32 b, uint32 a, RGB888* output)
    {
        output->r = r;
        output->g = g;
        output->b = b;
    }
};

struct UnpackRGBA16161616
{
    inline void operator()(const RGBA16161616* input, uint32& r, uint32& g, uint32& b, uint32& a)
    {
        r = input->r;
        g = input->g;
        b = input->b;
        a = input->a;
    }
};

struct PackRGBA16161616
{
    inline void operator()(uint32& r, uint32& g, uint32& b, uint32& a, RGBA16161616* out)
    {
        out->r = r;
        out->g = g;
        out->b = b;
        out->a = a;
    }
};

struct UnpackRGBA32323232
{
    inline void operator()(const RGBA32323232* input, uint64& r, uint64& g, uint64& b, uint64& a)
    {
        r = input->r;
        g = input->g;
        b = input->b;
        a = input->a;
    }
};

struct PackRGBA32323232
{
    inline void operator()(uint64& r, uint64& g, uint64& b, uint64& a, RGBA32323232* out)
    {
        out->r = static_cast<uint32>(r);
        out->g = static_cast<uint32>(g);
        out->b = static_cast<uint32>(b);
        out->a = static_cast<uint32>(a);
    }
};

struct UnpackRGBA5551
{
    inline void operator()(const uint16* input, uint32& r, uint32& g, uint32& b, uint32& a)
    {
        auto& in = *input;

        r = (in >> 11) & 0x001F;
        g = (in >> 6) & 0x001F;
        b = (in >> 1) & 0x001F;
        a = (in)&0x0001;
    }
};

struct PackRGBA16F
{
    inline void operator()(float32& r, float32& g, float32& b, float32& a, RGBA16F* out)
    {
        out->r = Float16Compressor::Compress(r);
        out->g = Float16Compressor::Compress(g);
        out->b = Float16Compressor::Compress(b);
        out->a = Float16Compressor::Compress(a);
    }
};

struct UnpackRGBA16F
{
    inline void operator()(const RGBA16F* input, float32& r, float32& g, float32& b, float32& a)
    {
        r = Float16Compressor::Decompress(input->r);
        g = Float16Compressor::Decompress(input->g);
        b = Float16Compressor::Decompress(input->b);
        a = Float16Compressor::Decompress(input->a);
    }
};

struct PackRGBA32F
{
    inline void operator()(float32& r, float32& g, float32& b, float32& a, RGBA32F* out)
    {
        out->r = r;
        out->g = g;
        out->b = b;
        out->a = a;
    }
};

struct UnpackRGBA32F
{
    inline void operator()(const RGBA32F* input, float32& r, float32& g, float32& b, float32& a)
    {
        r = input->r;
        g = input->g;
        b = input->b;
        a = input->a;
    }
};

struct PackRGBA5551
{
    inline void operator()(uint32 r, uint32 g, uint32 b, uint32 a, uint16* output)
    {
        *output = (r << 11) | (g << 6) | (b << 1) | a;
    }
};

struct PackNormalizedRGBA8888
{
    void operator()(uint32 r, uint32 g, uint32 b, uint32 a, uint32* output)
    {
        Vector3 v(r / 255.f, g / 255.f, b / 255.f);
        v *= 2.f;
        v -= Vector3(1.f, 1.f, 1.f);
        v.Normalize();
        v /= 2.f;
        v += Vector3(.5f, .5f, .5f);

        PackRGBA8888 packFunc;
        packFunc(uint32(0xFF * v.x), uint32(0xFF * v.y), uint32(0xFF * v.z), a, output);
    }
};

struct NormalizeRGBA8888
{
    inline void operator()(const uint32* input, uint32* output)
    {
        UnpackRGBA8888 unpack;
        PackNormalizedRGBA8888 pack;
        uint32 r, g, b, a;

        unpack(input, r, g, b, a);
        pack(r, g, b, a, output);
    }
};

struct NormalizeRGB16F
{
    inline void operator()(const RGB16F* input, RGB16F* output)
    {
        Vector3 nrm;
        nrm.x = Float16Compressor::Decompress(input->r);
        nrm.y = Float16Compressor::Decompress(input->g);
        nrm.z = Float16Compressor::Decompress(input->b);
        nrm.Normalize();
        output->r = Float16Compressor::Compress(nrm.x);
        output->g = Float16Compressor::Compress(nrm.y);
        output->b = Float16Compressor::Compress(nrm.z);
    }
};

struct NormalizeRGB32F
{
    inline void operator()(const RGB32F* input, RGB32F* output)
    {
        Vector3 nrm(input->r, input->g, input->b);
        nrm.Normalize();
        output->r = nrm.x;
        output->g = nrm.y;
        output->b = nrm.z;
    }
};

struct NormalizeRGBA16F
{
    inline void operator()(const RGBA16F* input, RGBA16F* output)
    {
        Vector4 nrm;
        nrm.x = Float16Compressor::Decompress(input->r);
        nrm.y = Float16Compressor::Decompress(input->g);
        nrm.z = Float16Compressor::Decompress(input->b);
        nrm.w = Float16Compressor::Decompress(input->a);
        nrm.Normalize();
        output->r = Float16Compressor::Compress(nrm.x);
        output->g = Float16Compressor::Compress(nrm.y);
        output->b = Float16Compressor::Compress(nrm.z);
        output->a = Float16Compressor::Compress(nrm.w);
    }
};

struct NormalizeRGBA32F
{
    inline void operator()(const RGBA32F* input, RGBA32F* output)
    {
        Vector4 nrm(input->r, input->g, input->b, input->a);
        nrm.Normalize();
        output->r = nrm.x;
        output->g = nrm.y;
        output->b = nrm.z;
        output->a = nrm.w;
    }
};

template <class TYPE_IN, class TYPE_OUT, typename CONVERT_FUNC>
class ConvertDirect
{
public:
    void operator()(const void* inData, uint32 width, uint32 height, uint32 pitch, void* outData)
    {
        CONVERT_FUNC func;
        const uint8* readPtr = reinterpret_cast<const uint8*>(inData);
        uint8* writePtr = reinterpret_cast<uint8*>(outData);

        for (uint32 y = 0; y < height; ++y)
        {
            const TYPE_IN* readPtrLine = reinterpret_cast<const TYPE_IN*>(readPtr);
            TYPE_OUT* writePtrLine = reinterpret_cast<TYPE_OUT*>(writePtr);
            for (uint32 x = 0; x < width; ++x)
            {
                func(readPtrLine, writePtrLine);
                readPtrLine++;
                writePtrLine++;
            }
            readPtr += pitch;
            writePtr += pitch;
        }
    };

    void operator()(void* data, uint32 width, uint32 height, uint32 pitch)
    {
        CONVERT_FUNC func;
        const uint8* ptrToLine = reinterpret_cast<const uint8*>(data);

        for (uint32 y = 0; y < height; ++y)
        {
            const TYPE_IN* ptr = reinterpret_cast<const TYPE_IN*>(ptrToLine);
            for (uint32 x = 0; x < width; ++x)
            {
                func(ptr, ptr);
                ptr++;
            }
            ptrToLine += pitch;
        }
    };

    void operator()(const void* inData, uint32 inWidth, uint32 inHeight, uint32 inPitch,
                    void* outData, uint32 outWidth, uint32 outHeight, uint32 outPitch)
    {
        CONVERT_FUNC func;
        const uint8* readPtr = reinterpret_cast<const uint8*>(inData);
        uint8* writePtr = reinterpret_cast<uint8*>(outData);

        for (uint32 y = 0; y < inHeight; ++y)
        {
            const TYPE_IN* readPtrLine = reinterpret_cast<const TYPE_IN*>(readPtr);
            TYPE_OUT* writePtrLine = reinterpret_cast<TYPE_OUT*>(writePtr);
            for (uint32 x = 0; x < inWidth; ++x)
            {
                func(readPtrLine, writePtrLine);
                readPtrLine++;
                writePtrLine++;
            }
            readPtr += inPitch;
            writePtr += outPitch;
        }
    };
};

template <class TYPE_IN, class TYPE_OUT, class CHANNEL_TYPE, typename UNPACK_FUNC, typename PACK_FUNC>
class ConvertDownscaleTwiceBillinear
{
public:
    void operator()(const void* inData, uint32 inWidth, uint32 inHeight, uint32 inPitch,
                    void* outData, uint32 outWidth, uint32 outHeight, uint32 outPitch)
#if defined(__DAVAENGINE_IPHONE__) && __clang_major__ == 7 && __clang_minor__ == 3
    // Clang 7.3 incorrectly optimizes nested loops below using ARM's vld2.32 instruction,
    // reading memory outside of input image's data bounds, which leads to EXC_BAD_ACCESS.
    // So, just disable any optimizations for this function on this specific clang version.
    __attribute__((optnone))
#endif
    {
        UNPACK_FUNC unpackFunc;
        PACK_FUNC packFunc;
        const uint8* readPtr = reinterpret_cast<const uint8*>(inData);
        uint8* writePtr = reinterpret_cast<uint8*>(outData);

        const uint32 lineStride = (inHeight > outHeight) ? inWidth : 0;
        const uint32 pixelStride = (inWidth > outWidth) ? 1 : 0;

        for (uint32 y = 0; y < outHeight; ++y)
        {
            const TYPE_IN* readPtrLine = reinterpret_cast<const TYPE_IN*>(readPtr);
            TYPE_OUT* writePtrLine = reinterpret_cast<TYPE_OUT*>(writePtr);

            for (uint32 x = 0; x < outWidth; ++x)
            {
                CHANNEL_TYPE r00, r01, r10, r11;
                CHANNEL_TYPE g00, g01, g10, g11;
                CHANNEL_TYPE b00, b01, b10, b11;
                CHANNEL_TYPE a00, a01, a10, a11;

                unpackFunc(readPtrLine, r00, g00, b00, a00);
                unpackFunc(readPtrLine + pixelStride, r01, g01, b01, a01);
                unpackFunc(readPtrLine + lineStride, r10, g10, b10, a10);
                unpackFunc(readPtrLine + lineStride + pixelStride, r11, g11, b11, a11);

                CHANNEL_TYPE r = (r00 + r01 + r10 + r11) / 4;
                CHANNEL_TYPE g = (g00 + g01 + g10 + g11) / 4;
                CHANNEL_TYPE b = (b00 + b01 + b10 + b11) / 4;
                CHANNEL_TYPE a = (a00 + a01 + a10 + a11) / 4;

                packFunc(r, g, b, a, writePtrLine);

                readPtrLine += 2;
                writePtrLine++;
            }
            readPtr += inPitch * 2;
            writePtr += outPitch;
        }
    };
};

class Image;

namespace ImageConvert
{
bool Normalize(PixelFormat format, const void* inData, uint32 width, uint32 height, uint32 pitch, void* outData);

bool ConvertImage(const Image* srcImage, Image* dstImage);
bool ConvertImageDirect(const Image* srcImage, Image* dstImage);
bool ConvertImageDirect(PixelFormat inFormat, PixelFormat outFormat,
                        const void* inData, uint32 inWidth, uint32 inHeight, uint32 inPitch,
                        void* outData, uint32 outWidth, uint32 outHeight, uint32 outPitch);

bool CanConvertFromTo(PixelFormat inFormat, PixelFormat outFormat);
bool CanConvertDirect(PixelFormat inFormat, PixelFormat outFormat);

void SwapRedBlueChannels(const Image* srcImage, const Image* dstImage = nullptr);
void SwapRedBlueChannels(PixelFormat format, void* srcData, uint32 width, uint32 height, uint32 pitch, void* dstData = nullptr);

Image* DownscaleTwiceBillinear(const Image* source, bool isNormalMap = false);
bool DownscaleTwiceBillinear(PixelFormat inFormat, PixelFormat outFormat,
                             const void* inData, uint32 inWidth, uint32 inHeight, uint32 inPitch,
                             void* outData, uint32 outWidth, uint32 outHeight, uint32 outPitch, bool normalize);

void ResizeRGBA8Billinear(const uint32* inPixels, uint32 w, uint32 h, uint32* outPixels, uint32 w2, uint32 h2);

bool ConvertFloatFormats(uint32 width, uint32 height, PixelFormat inFormat, PixelFormat outFormat, void* inData, void* outData);
};
};

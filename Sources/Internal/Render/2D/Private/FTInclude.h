#pragma once

#ifdef __DAVAENGINE_WIN_UAP__
#define generic GenericFromFreeTypeLibrary
#endif

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wold-style-cast"
#endif

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_CACHE_H

#ifdef __clang__
#pragma clang diagnostic pop
#endif

#ifdef __DAVAENGINE_WIN_UAP__
#undef generic
#endif
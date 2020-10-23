#ifndef __BS_COMMON_H__
#define __BS_COMMON_H__

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" { // only need to export C interface if
// used by C++ source code
#endif

typedef enum
{
    BS_PLAIN,
    BS_ZLIB
}
BSType;

struct bsdiff_stream
{
    void* opaque;
    BSType type;

    void* (*malloc)(int64_t size);
    void (*free)(void* ptr);
    int (*write)(struct bsdiff_stream* stream, const void* buffer, int64_t size);
};

struct bspatch_stream
{
    void* opaque;
    BSType type;

    int (*read)(const struct bspatch_stream* stream, void* buffer, int64_t size);
};

int bspatch(const uint8_t* olddata, int64_t oldsize, uint8_t* newdata, int64_t newsize, struct bspatch_stream* stream);
int bsdiff(const uint8_t* olddata, int64_t oldsize, const uint8_t* newdata, int64_t newsize, struct bsdiff_stream* stream);
void offtout(int64_t x, uint8_t* buf);
int64_t offtin(uint8_t* buf);

#ifdef __cplusplus
}
#endif

#endif // __BS_COMMON_H__

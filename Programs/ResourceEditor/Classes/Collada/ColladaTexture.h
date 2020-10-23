#ifndef __COLLADALOADER_COLLADATEXTURE_H__
#define __COLLADALOADER_COLLADATEXTURE_H__

#include "ColladaIncludes.h"

namespace DAVA
{
class ColladaTexture
{
public:
    ColladaTexture(FCDImage* image);
    ~ColladaTexture();

    bool PreLoad();

    GLuint GetTextureId()
    {
        return textureId;
    };

    fstring texturePathName;
    FCDImage* image = nullptr;
    GLuint textureId = 0;
    bool hasOpacity = false;
};
};

#endif // __COLLADALOADER_TEXTURE_H__

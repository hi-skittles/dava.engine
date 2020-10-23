#ifndef __COLLADALOADER_COLLADADOCUMENT_H__
#define __COLLADALOADER_COLLADADOCUMENT_H__

#include "ColladaIncludes.h"
#include "ColladaMesh.h"
#include "ColladaScene.h"
#include "ColladaErrorCodes.h"

#include "DAVAEngine.h"

namespace DAVA
{
class ColladaDocument
{
public:
    eColladaErrorCodes Open(const char* filename, bool animationsOnly = false);
    bool ExportAnimations(const char* filename);
    bool ExportNodeAnimations(FCDocument* exportDoc, FCDSceneNode* exportNode);

    void Close();

    void Render();

    eColladaErrorCodes SaveSC2(const FilePath& scenePath) const;
    eColladaErrorCodes SaveAnimations(const FilePath& dir) const;
    String GetTextureName(const FilePath& scenePath, ColladaTexture* texture);

    void GetAnimationTimeInfo(FCDocument* document, float32& timeStart, float32& timeEnd);

    FILE* sceneFP = nullptr;
    ColladaScene* colladaScene = nullptr;

private:
    FCDocument* document = nullptr;
};
};

#endif
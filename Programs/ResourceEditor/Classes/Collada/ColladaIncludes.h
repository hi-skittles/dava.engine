#pragma once

#include "Base/BaseMath.h"

#include "FCollada.h"

#include "FCDocument/FCDocument.h"
#include "FCDocument/FCDocumentTools.h"
#include "FCDocument/FCDLibrary.h"
#include "FCDocument/FCDGeometry.h"
#include "FCDocument/FCDImage.h"
#include "FCDocument/FCDSceneNode.h"
#include "FCDocument/FCDTransform.h"

#include "FCDocument/FCDGeometryMesh.h"
#include "FCDocument/FCDGeometryPolygons.h"
#include "FCDocument/FCDGeometrySource.h"
#include "FCDocument/FCDGeometryPolygonsTools.h"
#include "FCDocument/FCDGeometryPolygonsInput.h"
#include "FCDocument/FCDGeometryInstance.h"

#include "FCDocument/FCDLight.h"
#include "FCDocument/FCDMaterial.h"
#include "FCDocument/FCDMaterialInstance.h"
#include "FCDocument/FCDController.h"
#include "FCDocument/FCDControllerInstance.h"
#include "FCDocument/FCDSkinController.h"
#include "FCDocument/FCDAnimated.h"
#include "FCDocument/FCDAnimationCurve.h"
#include "FCDocument/FCDAnimation.h"
#include "FCDocument/FCDAnimationChannel.h"
#include "FCDocument/FCDAnimationKey.h"
#include "FCDocument/FCDCamera.h"

#include <FMath/FMMatrix44.h>

#define COLLADA_GLUT_RENDER

#ifdef COLLADA_GLUT_RENDER

    #if defined(__DAVAENGINE_WIN32__)
        #include <GL/glew.h>
    #endif

    #if defined(_WIN32)
// Disable automatic linking with glut library
        #define GLUT_NO_LIB_PRAGMA
        #include <GL/glut.h>
        #include <direct.h>
        #define getcwd _getcwd
    #elif defined(__APPLE__) || defined(MACOSX)
        #include <GLUT/glut.h>
    #endif

#endif // COLLADA_GLUT_RENDER

namespace DAVA
{
Matrix4 ConvertMatrix(FMMatrix44& matrix);
Matrix4 ConvertMatrixT(FMMatrix44& matrix);
};

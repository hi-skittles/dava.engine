#ifndef __PARTICLES_EDITOR_NODE_NAME_HELPER_H__
#define __PARTICLES_EDITOR_NODE_NAME_HELPER_H__

#include "DAVAEngine.h"

namespace DAVA
{
// Scene Data Helper for new node and layers names
class ParticlesEditorNodeNameHelper
{
public:
    static String GetNewNodeName(const String& name, Entity* parentNode);
    static String GetNewLayerName(const String& name, ParticleEmitter* emitter);

private:
    static String GetBaseName(const String& name);
    static bool IsNodeNameExist(const String& name, Entity* parentNode);

    static bool IsLayerNameExist(const String& name, ParticleEmitter* emitter);
};
};

#endif /* defined(__PARTICLES_EDITOR_NODE_NAME_HELPER_H__) */

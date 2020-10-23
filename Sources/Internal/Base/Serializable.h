#ifndef __DAVAENGINE_SIRIALIZABLE__H__
#define __DAVAENGINE_SIRIALIZABLE__H__

#include "Base/BaseTypes.h"
#include "Scene3D/SceneFile/SerializationContext.h"

namespace DAVA
{
class KeyedArchive;

class Serializable
{
public:
    virtual ~Serializable() = default;
    virtual void Serialize(KeyedArchive* archive, SerializationContext* serializationContext) = 0;
    virtual void Deserialize(KeyedArchive* archive, SerializationContext* serializationContext) = 0;
};

} // namespace DAVA

#endif // __DAVAENGINE_SIRIALIZABLE__H__

#pragma once

#include <Functional/Function.h>
#include <Base/BaseTypes.h>

namespace DAVA
{
class Landscape;
class Texture;

namespace LandscapeThumbnails
{
using Callback = Function<void(Landscape*, Texture*)>;
using RequestID = uint32;

const RequestID InvalidID = 0;

RequestID Create(Landscape* landscape, Callback callback);
void CancelRequest(RequestID);
}
}

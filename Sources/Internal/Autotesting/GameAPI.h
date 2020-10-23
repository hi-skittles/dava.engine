#ifndef __DAVAENGINE_GAME_API_H__
#define __DAVAENGINE_GAME_API_H__

#include "DAVAConfig.h"

#include "Base/Singleton.h"

#ifdef __DAVAENGINE_AUTOTESTING__

namespace DAVA
{
class GameAPI : public Singleton<GameAPI>
{
public:
};
}

#endif //__DAVAENGINE_AUTOTESTING__

#endif //__DAVAENGINE_GAME_API_H__

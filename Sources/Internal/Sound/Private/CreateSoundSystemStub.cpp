#include "Sound/SoundSystem.h"

namespace DAVA
{
SoundSystem* CreateSoundSystem(Engine* e)
{
    static SoundSystem* instSoundSystem = nullptr;
    if (nullptr == instSoundSystem)
    {
        instSoundSystem = new SoundSystem(e);
    }
    return instSoundSystem;
}
}

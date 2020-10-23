#ifndef __GRAPHICS_DETECT_H__
#define __GRAPHICS_DETECT_H__

#include "DAVAEngine.h"
#include "GraphicsLevel.h"

class GraphicsDetect : public DAVA::Singleton<GraphicsDetect>
{
public:
    GraphicsDetect();
    ~GraphicsDetect();

    void OverrideSettings(const DAVA::String group);
    DAVA::String GetLevelString();
    void ReloadSettings();
    GraphicsLevel* GetLevel(void)
    {
        return activeGroup->level;
    };

private:
    struct Group
    {
        DAVA::String base;
        DAVA::Set<DAVA::String> devices;
        GraphicsLevel* level;
    };

    Group* GetAutoDetected();
    void Init();

    DAVA::Map<DAVA::String, Group> groupsMap;
    Group* activeGroup;
};
#endif
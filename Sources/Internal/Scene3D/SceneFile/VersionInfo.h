#ifndef __DAVAENGINE_SCENEVERSION_H__
#define __DAVAENGINE_SCENEVERSION_H__

#include "Base/BaseTypes.h"
#include "Base/BaseObject.h"
#include "Base/FastName.h"
#include "Base/Singleton.h"
#include "FileSystem/FilePath.h"

//#define USER_VERSIONING_DEBUG_FEATURES

namespace DAVA
{
static const int32 CUSTOM_PROPERTIES_COMPONENT_SAVE_SCENE_VERSION = 8;
static const int32 OLD_LODS_SCENE_VERSION = 11;
static const int32 TREE_ANIMATION_SCENE_VERSION = 12;
static const int32 PREREQUIRED_BINORMAL_SCENE_VERSION = 13;
static const int32 SHADOW_VOLUME_SCENE_VERSION = 15;
static const int32 DEPRECATED_MATERIAL_FLAGS_SCENE_VERSION = 16;
static const int32 ALPHATEST_VALUE_FLAG_SCENE_VERSION = 17;
static const int32 RHI_SCENE_VERSION = 18;
static const int32 FIXED_VEGETATION_SCENE_VERSION = 19;
static const int32 LODSYSTEM2 = 20;
static const int32 OLD_MATERIAL_FLAGS_SCENE_VERSION = 21;
static const int32 SPEED_TREE_POLYGON_GROUPS_PIVOT3_SCENE_VERSION = 22; // convert EVF_PIVOT -> EVF_PIVOT4; EVF_PIVOT depricated
static const int32 COMPONENTS_REFLECTION_SCENE_VERSION = 23; // enum Component::eType removed, scene components serialization without "comp.type".
static const int32 TRANSFORM_REFACTORING_SCENE_VERSION = 24; // TransformComponent has Transform instead of Matrix4

static const int32 SCENE_FILE_CURRENT_VERSION = TRANSFORM_REFACTORING_SCENE_VERSION;
static const int32 SCENE_FILE_MINIMAL_SUPPORTED_VERSION = 9;

class VersionInfo
: public Singleton<VersionInfo>
{
public:
    using TagsMap = Map<String, uint32>;
    struct SceneVersion
    {
        uint32 version;
        TagsMap tags;

        SceneVersion() //-V730 no need to init tags
        : version(0)
        {
        }
        bool IsValid() const
        {
            return version > 0;
        }
    };
    using VersionMap = Map<uint32, SceneVersion>;

    enum eStatus
    {
        VALID,
        COMPATIBLE,
        INVALID,
    };

public:
    VersionInfo();
    ~VersionInfo();

    const SceneVersion& GetCurrentVersion() const;
    eStatus TestVersion(const SceneVersion& version) const;
    String UnsupportedTagsMessage(const SceneVersion& version) const;
    String NoncompatibleTagsMessage(const SceneVersion& version) const;

#ifdef USER_VERSIONING_DEBUG_FEATURES
    VersionMap& Versions();
    VersionMap GetDefaultVersionHistory();
#else
    const VersionMap& Versions() const;
#endif

    static void AddVersion(VersionMap& versions, const SceneVersion& version);

private:
    VersionMap GetVersionHistory();
    void SetCurrentBranch();
    TagsMap GetTags(uint32 minVersion = 0) const;

    VersionMap versionMap;

    static TagsMap GetTagsDiff(const TagsMap& from, const TagsMap& what);
    static String FormatTagsString(const TagsMap& tags);
};
};

#endif

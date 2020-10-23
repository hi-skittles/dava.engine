#include "VersionInfo.h"
#include "Utils/StringFormat.h"
#include "SerializationContext.h"

#include <sstream>

namespace DAVA
{
VersionInfo::VersionInfo()
{
    versionMap = GetVersionHistory();
    SetCurrentBranch();
}

VersionInfo::~VersionInfo()
{
}

void VersionInfo::AddVersion(VersionMap& versions, const SceneVersion& version)
{
    DVASSERT(versions.find(version.version) == versions.end());
    versions.insert(VersionMap::value_type(version.version, version));
}

const VersionInfo::SceneVersion& VersionInfo::GetCurrentVersion() const
{
    DVASSERT(!versionMap.empty());
    return versionMap.rbegin()->second;
}

String VersionInfo::UnsupportedTagsMessage(const SceneVersion& version) const
{
    const TagsMap& allTags = GetTags();
    const TagsMap& errTags = GetTagsDiff(allTags, version.tags); // List of tags that not supported by current version of framework
    const String& msg = FormatTagsString(errTags);

    return msg;
}

String VersionInfo::NoncompatibleTagsMessage(const SceneVersion& version) const
{
    const TagsMap& allTags = GetTags(version.version);
    const TagsMap& warnTags = GetTagsDiff(version.tags, allTags); // List of tags that will be added to scene
    const String& msg = FormatTagsString(warnTags);

    return msg;
}

VersionInfo::TagsMap VersionInfo::GetTagsDiff(const TagsMap& from, const VersionInfo::TagsMap& what)
{
    TagsMap result;

    for (TagsMap::const_iterator it = from.begin(); it != from.end(); it++)
    {
        if (what.find(it->first) == what.end())
        {
            result.insert(TagsMap::value_type(it->first, it->second));
        }
    }

    return result;
}

String VersionInfo::FormatTagsString(const TagsMap& tags)
{
    StringStream ss;
    for (TagsMap::const_iterator it = tags.begin(); it != tags.end(); ++it)
    {
        ss << it->first << " (" << it->second << ")" << std::endl;
    }

    return ss.str();
}

VersionInfo::TagsMap VersionInfo::GetTags(uint32 minVersion) const
{
    TagsMap tags;

    for (VersionMap::const_iterator itVersion = versionMap.begin(); itVersion != versionMap.end(); ++itVersion)
    {
        if (itVersion->first < minVersion)
            continue;

        const SceneVersion& version = itVersion->second;
        tags.insert(version.tags.begin(), version.tags.end());
    }

    return tags;
}

VersionInfo::eStatus VersionInfo::TestVersion(const SceneVersion& version) const
{
    const SceneVersion& current = GetCurrentVersion();

    // Checking version
    if (current.version < version.version)
        return INVALID;

    // Checking tags
    const TagsMap& tags = version.tags;
    const TagsMap& fwAllTags = GetTags();
    const TagsMap& fwVersionedTags = GetTags(version.version);

    const TagsMap& errTags = GetTagsDiff(tags, fwAllTags); // List of tags that not supported by current version of framework
    const TagsMap& warnTags = GetTagsDiff(fwVersionedTags, tags); // List of tags that will be added to scene

    if (errTags.size() > 0)
        return INVALID;

    if (warnTags.size() > 0)
        return COMPATIBLE;

    return VALID;
}

VersionInfo::VersionMap VersionInfo::GetVersionHistory()
{
    VersionMap versions;

    // Current version
    SceneVersion currentVersion;
    currentVersion.version = SCENE_FILE_CURRENT_VERSION; // Current version of scene file
    AddVersion(versions, currentVersion);

    return versions;
}

void VersionInfo::SetCurrentBranch()
{
    // List of featues, that are under development in current branch

    DVASSERT(!versionMap.empty());

    // Example:
    // TagsMap& tags = versionMap.rbegin()->second.tags;
    // tags.insert(TagsMap::value_type("sky", 2));
}

#ifdef USER_VERSIONING_DEBUG_FEATURES
VersionInfo::VersionMap& VersionInfo::Versions()
#else
const VersionInfo::VersionMap& VersionInfo::Versions() const
#endif
{
    return versionMap;
}

#ifdef USER_VERSIONING_DEBUG_FEATURES
VersionInfo::VersionMap VersionInfo::GetDefaultVersionHistory()
{
    return GetVersionHistory();
}
#endif
}

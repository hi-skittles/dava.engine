#ifndef __DAVAENGINE_PARTICLES_QUALITY_SETTINGS_H__
#define __DAVAENGINE_PARTICLES_QUALITY_SETTINGS_H__

#include "Base/BaseTypes.h"
#include "Base/FastName.h"
#include "FileSystem/FilePath.h"

namespace DAVA
{
class YamlNode;
class ParticlesQualitySettings
{
public:
    class QualitySheet
    {
    public:
        struct Selector
        {
            UnorderedSet<FastName> qualities;
            UnorderedSet<FastName> tags;
        };

        struct Action
        {
            bool Apply(const FilePath& originalPath, FilePath& alternativePath) const;

            FastName name;
            Vector<String> params;
        };

        QualitySheet();
        QualitySheet(const QualitySheet& src);
        QualitySheet(const Selector& selector, const Vector<Action>& actions, uint32 qualitiesCount);

        const Selector& GetSelector() const
        {
            return selector;
        }
        const Vector<Action>& GetActions() const
        {
            return actions;
        }

        bool IsMatched(const FastName& quality, const Set<FastName>& tagsCloud) const;
        bool Apply(const FilePath& originalPath, FilePath& alternativePath) const;

        int32 GetScore() const;

    private:
        void RecalculateScore();

        Selector selector;
        Vector<Action> actions;
        uint32 qualitiesCount = 0;
        uint32 score = 0;
    };

    class FilepathSelector
    {
    public:
        FilepathSelector(const ParticlesQualitySettings& settings);
        FilePath SelectFilepath(const FilePath& originalFilepath) const;

    private:
        Vector<QualitySheet> actualSheets;
    };

    ParticlesQualitySettings();
    ~ParticlesQualitySettings();

    void LoadFromYaml(const YamlNode* root);

    size_t GetQualitiesCount() const;
    FastName GetQualityName(size_t index) const;
    int32 GetQualityIndex(const FastName& name) const;

    FastName GetCurrentQuality() const;
    void SetCurrentQuality(const FastName& name);

    const Set<FastName>& GetTagsCloud() const;
    void SetTagsCloud(const Set<FastName>& newTagsCloud);

    bool HasTag(const FastName& tag) const;
    void AddTag(const FastName& tag);
    void RemoveTag(const FastName& tag);

    const Vector<QualitySheet>& GetQualitySheets() const
    {
        return qualitySheets;
    }

    const FilepathSelector* GetOrCreateFilepathSelector();

private:
    Vector<FastName> qualities;
    int32 defaultQualityIndex = -1;
    int32 currentQualityIndex = -1;
    Vector<QualitySheet> qualitySheets;
    Set<FastName> tagsCloud;

    std::unique_ptr<FilepathSelector> filepathSelector;
};
};
#endif // __DAVAENGINE_PARTICLES_QUALITY_SETTINGS_H__

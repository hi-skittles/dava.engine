#ifndef __DAVAENGINE_UI_PACKAGE_LOADER_H__
#define __DAVAENGINE_UI_PACKAGE_LOADER_H__
#include "Base/BaseObject.h"
#include "Base/BaseTypes.h"
#include "Base/BaseMath.h"
#include "UI/AbstractUIPackageBuilder.h"

namespace DAVA
{
class UIYamlLoader;
class UIControl;
class YamlNode;
class FilePath;
class UIPackage;
class UIControlBackground;

class UIPackageLoader : public AbstractUIPackageLoader
{
public:
    static const DAVA::int32 MIN_SUPPORTED_VERSION = 0;

    static const DAVA::int32 VERSION_WITH_LEGACY_ALIGNS = 0;
    static const DAVA::int32 LAST_VERSION_WITH_LINEAR_LAYOUT_LEGACY_ORIENTATION = 1;
    static const DAVA::int32 LAST_VERSION_WITH_LEGACY_SPRITE_MODIFICATION = 2;
    static const DAVA::int32 LAST_VERSION_WITHOUT_PROTOTYPES_SUPPORT = 5;
    static const DAVA::int32 LAST_VERSION_WITH_LEGACY_DEBUG_DRAW = 14;
    static const DAVA::int32 LAST_VERSION_WITH_LEGACY_CLIP_CONTENT = 14;
    static const DAVA::int32 LAST_VERSION_WITH_RICH_SINGLE_ALISES = 15;
    static const DAVA::int32 LAST_VERSION_WITH_LEGACY_STATIC_TEXT = 17;

public:
    UIPackageLoader();
    UIPackageLoader(const DAVA::Map<DAVA::String, DAVA::Set<DAVA::FastName>>& legacyPrototypes);
    virtual ~UIPackageLoader();

public:
    virtual bool LoadPackage(const FilePath& packagePath, AbstractUIPackageBuilder* builder) override;
    virtual bool LoadPackage(const YamlNode* rootNode, const FilePath& packagePath, AbstractUIPackageBuilder* builder);
    virtual bool LoadControlByName(const FastName& name, AbstractUIPackageBuilder* builder) override;

private:
    struct ComponentNode
    {
        const YamlNode* node;
        const Type* type;
        uint32 index;
    };

private:
    void LoadStyleSheets(const YamlNode* styleSheetsNode, AbstractUIPackageBuilder* builder);
    void LoadControl(const YamlNode* node, AbstractUIPackageBuilder::eControlPlace controlPlace, AbstractUIPackageBuilder* builder);

    void LoadControlPropertiesFromYamlNode(const ReflectedType* ref, const YamlNode* node, AbstractUIPackageBuilder* builder);

    void LoadComponentPropertiesFromYamlNode(const YamlNode* node, AbstractUIPackageBuilder* builder);
    void LoadBindingsFromYamlNode(const YamlNode* node, AbstractUIPackageBuilder* builder);
    void ProcessLegacyAligns(const YamlNode* node, AbstractUIPackageBuilder* builder) const;
    void ProcessLegacyDebugDraw(const YamlNode* node, AbstractUIPackageBuilder* builder) const;
    void ProcessLegacyClipContent(const YamlNode* node, AbstractUIPackageBuilder* builder) const;
    void ProcessLegacyRichSingleAliases(const YamlNode* node, AbstractUIPackageBuilder* builder) const;
    void ProcessLegacyStaticText(const ReflectedType* ref, const YamlNode* node, AbstractUIPackageBuilder* builder) const;
    Vector<ComponentNode> ExtractComponentNodes(const YamlNode* node);

    Any ReadAnyFromYamlNode(const ReflectedStructure::Field* fieldRef, const YamlNode* node, const String& name) const;

private:
    enum eItemStatus
    {
        STATUS_WAIT,
        STATUS_LOADING,
        STATUS_LOADED
    };

    struct QueueItem
    {
        FastName name;
        const YamlNode* node;
        int32 status;
    };

    Vector<QueueItem> loadingQueue;
    DAVA::int32 version = 0;

    DAVA::Map<DAVA::String, DAVA::String> legacyAlignsMap;
    DAVA::Map<DAVA::String, DAVA::String> legacyDebugDrawMap;
    DAVA::Map<DAVA::String, DAVA::Set<DAVA::FastName>> legacyPrototypes;
    DAVA::Map<DAVA::String, DAVA::String> legacyStaticTextMap;
};
};

#endif // __DAVAENGINE_UI_PACKAGE_LOADER_H__

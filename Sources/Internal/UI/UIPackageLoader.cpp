#include "UIPackageLoader.h"

#include "Base/ObjectFactory.h"
#include "Entity/ComponentManager.h"
#include "Engine/Engine.h"
#include "FileSystem/FileSystem.h"
#include "FileSystem/FilePath.h"
#include "FileSystem/YamlEmitter.h"
#include "FileSystem/YamlNode.h"
#include "FileSystem/YamlParser.h"
#include "FileSystem/YamlParser.h"
#include "UI/UIControl.h"
#include "UI/Styles/UIStyleSheet.h"
#include "UI/UIStaticText.h"
#include "UI/Text/UITextComponent.h"
#include "UI/UIControlHelpers.h"
#include "UI/UIPackage.h"
#include "UI/Components/UIComponent.h"
#include "UI/DataBinding/UIDataBindingComponent.h"
#include "UI/Layouts/UIAnchorComponent.h"
#include "UI/Layouts/UILinearLayoutComponent.h"
#include "UI/Render/UIDebugRenderComponent.h"
#include "UI/Render/UIClipContentComponent.h"
#include "UI/RichContent/UIRichContentAliasesComponent.h"
#include "Utils/Utils.h"
#include "Logger/Logger.h"
#include "Reflection/ReflectedTypeDB.h"

namespace DAVA
{
UIPackageLoader::UIPackageLoader()
    : UIPackageLoader(Map<String, Set<FastName>>())
{
}

UIPackageLoader::UIPackageLoader(const Map<String, DAVA::Set<FastName>>& legacyPrototypes_)
    : legacyPrototypes(legacyPrototypes_)
{
    version = DAVA::UIPackage::CURRENT_VERSION;
    if (MIN_SUPPORTED_VERSION <= VERSION_WITH_LEGACY_ALIGNS)
    {
        legacyAlignsMap["leftAnchorEnabled"] = "leftAlignEnabled";
        legacyAlignsMap["leftAnchor"] = "leftAlign";
        legacyAlignsMap["hCenterAnchorEnabled"] = "hcenterAlignEnabled";
        legacyAlignsMap["hCenterAnchor"] = "hcenterAlign";
        legacyAlignsMap["rightAnchorEnabled"] = "rightAlignEnabled";
        legacyAlignsMap["rightAnchor"] = "rightAlign";
        legacyAlignsMap["topAnchorEnabled"] = "topAlignEnabled";
        legacyAlignsMap["topAnchor"] = "topAlign";
        legacyAlignsMap["vCenterAnchorEnabled"] = "vcenterAlignEnabled";
        legacyAlignsMap["vCenterAnchor"] = "vcenterAlign";
        legacyAlignsMap["bottomAnchorEnabled"] = "bottomAlignEnabled";
        legacyAlignsMap["bottomAnchor"] = "bottomAlign";
    }
    else
    {
        DVASSERT(false); // we have to remove legacy aligns support if min supported version more than version with legacy aligns
    }

    legacyDebugDrawMap["enabled"] = "debugDraw";
    legacyDebugDrawMap["drawColor"] = "debugDrawColor";

    // Map old properties from UIStaticText to UITextConponent (except "fitting" and "multiline")
    legacyStaticTextMap["colorInheritType"] = "textcolorInheritType";
    legacyStaticTextMap["align"] = "textalign";
    legacyStaticTextMap["color"] = "textColor";
    legacyStaticTextMap["fontName"] = "font";
    legacyStaticTextMap["forceBiDiSupport"] = "forceBiDiSupport";
    legacyStaticTextMap["perPixelAccuracyType"] = "textperPixelAccuracyType";
    legacyStaticTextMap["shadowColor"] = "shadowcolor";
    legacyStaticTextMap["shadowOffset"] = "shadowoffset";
    legacyStaticTextMap["text"] = "text";
    legacyStaticTextMap["useRtlAlign"] = "textUseRtlAlign";
}

UIPackageLoader::~UIPackageLoader()
{
}

bool UIPackageLoader::LoadPackage(const FilePath& packagePath, AbstractUIPackageBuilder* builder)
{
    if (!loadingQueue.empty())
    {
        DVASSERT(false);
        loadingQueue.clear();
    }

    if (!FileSystem::Instance()->Exists(packagePath))
        return false;

    RefPtr<YamlParser> parser(YamlParser::Create(packagePath));
    if (!parser.Valid())
        return false;

    YamlNode* rootNode = parser->GetRootNode();
    if (!rootNode) //empty yaml equal to empty UIPackage
    {
        builder->BeginPackage(packagePath, UIPackage::CURRENT_VERSION);
        builder->EndPackage();
        return true;
    }

    return LoadPackage(rootNode, packagePath, builder);
}

bool UIPackageLoader::LoadPackage(const YamlNode* rootNode, const FilePath& packagePath, AbstractUIPackageBuilder* builder)
{
    const YamlNode* headerNode = rootNode->Get("Header");
    if (!headerNode)
        return false;

    const YamlNode* versionNode = headerNode->Get("version");
    if (versionNode == nullptr || versionNode->GetType() != YamlNode::TYPE_STRING)
    {
        return false;
    }

    int32 packageVersion = versionNode->AsInt();
    if (packageVersion < MIN_SUPPORTED_VERSION || UIPackage::CURRENT_VERSION < packageVersion)
    {
        return false;
    }

    builder->BeginPackage(packagePath, packageVersion);

    const YamlNode* importedPackagesNode = rootNode->Get("ImportedPackages");
    if (importedPackagesNode)
    {
        int32 count = static_cast<int32>(importedPackagesNode->GetCount());
        for (int32 i = 0; i < count; i++)
            builder->ProcessImportedPackage(importedPackagesNode->Get(i)->AsString(), this);
    }

    version = packageVersion; // store version in instance variables after importing packages

    const YamlNode* styleSheetsNode = rootNode->Get("StyleSheets");
    if (styleSheetsNode)
    {
        LoadStyleSheets(styleSheetsNode, builder);
    }

    const YamlNode* prototypesNode = rootNode->Get("Prototypes");
    if (prototypesNode)
    {
        int32 count = static_cast<int32>(prototypesNode->GetCount());
        for (int32 i = 0; i < count; i++)
        {
            const YamlNode* node = prototypesNode->Get(i);
            QueueItem item;
            item.name = node->Get("name")->AsFastName();
            item.node = node;
            item.status = STATUS_WAIT;
            loadingQueue.push_back(item);
        }

        for (int32 i = 0; i < count; i++)
        {
            if (loadingQueue[i].status == STATUS_WAIT)
            {
                loadingQueue[i].status = STATUS_LOADING;
                LoadControl(loadingQueue[i].node, AbstractUIPackageBuilder::TO_PROTOTYPES, builder);
                loadingQueue[i].status = STATUS_LOADED;
            }
        }

        loadingQueue.clear();
    }

    const YamlNode* controlsNode = rootNode->Get("Controls");
    if (controlsNode)
    {
        int32 count = static_cast<int32>(controlsNode->GetCount());
        for (int32 i = 0; i < count; i++)
        {
            const YamlNode* node = controlsNode->Get(i);
            QueueItem item;
            item.name = node->Get("name")->AsFastName();
            item.node = node;
            item.status = STATUS_WAIT;
            loadingQueue.push_back(item);
        }

        for (int32 i = 0; i < count; i++)
        {
            if (loadingQueue[i].status == STATUS_WAIT)
            {
                loadingQueue[i].status = STATUS_LOADING;
                AbstractUIPackageBuilder::eControlPlace controlPlace = AbstractUIPackageBuilder::TO_CONTROLS;
                if (version <= LAST_VERSION_WITHOUT_PROTOTYPES_SUPPORT)
                {
                    auto it = legacyPrototypes.find(packagePath.GetFrameworkPath());
                    if (it != legacyPrototypes.end())
                    {
                        if (it->second.find(loadingQueue[i].name) != it->second.end())
                        {
                            controlPlace = AbstractUIPackageBuilder::TO_PROTOTYPES;
                        }
                    }
                }

                LoadControl(loadingQueue[i].node, controlPlace, builder);
                loadingQueue[i].status = STATUS_LOADED;
            }
        }

        loadingQueue.clear();
    }

    const YamlNode* customDataNode = rootNode->Get("CustomData");
    if (customDataNode != nullptr)
    {
        builder->ProcessCustomData(customDataNode);
    }

    builder->EndPackage();

    return true;
}

bool UIPackageLoader::LoadControlByName(const FastName& name, AbstractUIPackageBuilder* builder)
{
    size_t size = loadingQueue.size();
    for (size_t index = 0; index < size; index++)
    {
        if (loadingQueue[index].name == name)
        {
            switch (loadingQueue[index].status)
            {
            case STATUS_WAIT:
                loadingQueue[index].status = STATUS_LOADING;
                LoadControl(loadingQueue[index].node, AbstractUIPackageBuilder::TO_PROTOTYPES, builder);
                loadingQueue[index].status = STATUS_LOADED;
                return true;

            case STATUS_LOADED:
                return true;

            case STATUS_LOADING:
                return false;

            default:
                DVASSERT(false);
                return false;
            }
        }
    }
    return false;
}

void UIPackageLoader::LoadStyleSheets(const YamlNode* styleSheetsNode, AbstractUIPackageBuilder* builder)
{
    const auto& styleSheetMap = styleSheetsNode->AsVector();
    const UIStyleSheetPropertyDataBase* propertyDB = UIStyleSheetPropertyDataBase::Instance();

    for (const auto& styleSheetNode : styleSheetMap)
    {
        const YamlNode* properties = styleSheetNode->Get("properties");

        if (properties != nullptr)
        {
            Vector<UIStyleSheetProperty> propertiesToSet;

            for (uint32 propertyIndex = 0; propertyIndex < properties->GetCount(); propertyIndex++)
            {
                FastName propertyName(properties->GetItemKeyName(propertyIndex));
                if (propertyDB->IsValidStyleSheetProperty(propertyName))
                {
                    uint32 index = propertyDB->GetStyleSheetPropertyIndex(propertyName);
                    const UIStyleSheetPropertyDescriptor& propertyDescr = propertyDB->GetStyleSheetPropertyByIndex(index);
                    if (propertyDescr.field != nullptr)
                    {
                        const YamlNode* propertyNode = properties->Get(propertyIndex);
                        const YamlNode* valueNode = propertyNode;
                        if (propertyNode->GetType() == YamlNode::TYPE_MAP)
                            valueNode = propertyNode->Get("value");

                        if (valueNode)
                        {
                            Any value(valueNode->AsAny(propertyDescr.field));

                            UIStyleSheetProperty property{ index, value };

                            if (propertyNode->GetType() == YamlNode::TYPE_MAP)
                            {
                                const YamlNode* transitionTime = propertyNode->Get("transitionTime");
                                if (transitionTime)
                                {
                                    property.transition = true;
                                    property.transitionTime = transitionTime->AsFloat();

                                    const YamlNode* transitionFunction = propertyNode->Get("transitionFunction");
                                    if (transitionFunction)
                                    {
                                        int32 transitionFunctionType = Interpolation::LINEAR;
                                        GlobalEnumMap<Interpolation::FuncType>::Instance()->ToValue(transitionFunction->AsString().c_str(), transitionFunctionType);
                                        property.transitionFunction = static_cast<Interpolation::FuncType>(transitionFunctionType);
                                    }
                                }
                            }

                            propertiesToSet.push_back(property);
                        }
                        else
                        {
                            DVASSERT(valueNode);
                        }
                    }
                }
                else
                {
                    Logger::Error("Unknown property name: %s", propertyName.c_str());
                    DVASSERT(false);
                }
            }

            Vector<String> selectorList;
            Split(styleSheetNode->Get("selector")->AsString(), ",", selectorList);
            Vector<UIStyleSheetSelectorChain> selectorChains;
            selectorChains.reserve(selectorList.size());

            for (const String& selectorString : selectorList)
            {
                selectorChains.push_back(UIStyleSheetSelectorChain(selectorString));
            }

            builder->ProcessStyleSheet(selectorChains, propertiesToSet);
        }
    }
}

void UIPackageLoader::LoadControl(const YamlNode* node, AbstractUIPackageBuilder::eControlPlace controlPlace, AbstractUIPackageBuilder* builder)
{
    const ReflectedType* controlReflectedType = nullptr;

    const YamlNode* pathNode = node->Get("path");
    const YamlNode* prototypeNode = node->Get("prototype");
    const YamlNode* classNode = node->Get("class");
    const YamlNode* nameNode = node->Get("name");

    DVASSERT(nameNode || pathNode);

    FastName controlName;
    if (nameNode)
    {
        controlName = nameNode->AsFastName();
    }

    if (pathNode)
    {
        controlReflectedType = builder->BeginControlWithPath(pathNode->AsString());
    }
    else if (prototypeNode)
    {
        const YamlNode* customClassNode = node->Get("customClass");
        const String* customClass = customClassNode == nullptr ? nullptr : &(customClassNode->AsString());
        String prototypeName = prototypeNode->AsString();
        String packageName = "";
        size_t pos = prototypeName.find('/');
        if (pos != String::npos)
        {
            packageName = prototypeName.substr(0, pos);
            prototypeName = prototypeName.substr(pos + 1, prototypeName.length() - pos - 1);
        }
        controlReflectedType = builder->BeginControlWithPrototype(controlName, packageName, FastName(prototypeName), customClass, this);
    }
    else if (classNode)
    {
        const YamlNode* customClassNode = node->Get("customClass");
        if (customClassNode)
            controlReflectedType = builder->BeginControlWithCustomClass(controlName, customClassNode->AsString(), classNode->AsString());
        else
            controlReflectedType = builder->BeginControlWithClass(controlName, classNode->AsString());
    }
    else
    {
        builder->BeginUnknownControl(controlName, node);
    }

    if (controlReflectedType != nullptr)
    {
        LoadControlPropertiesFromYamlNode(controlReflectedType, node, builder);
        LoadComponentPropertiesFromYamlNode(node, builder);
        LoadBindingsFromYamlNode(node, builder);

        if (version <= VERSION_WITH_LEGACY_ALIGNS)
        {
            ProcessLegacyAligns(node, builder);
        }

        if (version <= LAST_VERSION_WITH_LEGACY_DEBUG_DRAW)
        {
            ProcessLegacyDebugDraw(node, builder);
        }

        if (version <= LAST_VERSION_WITH_LEGACY_CLIP_CONTENT)
        {
            ProcessLegacyClipContent(node, builder);
        }

        if (version <= LAST_VERSION_WITH_RICH_SINGLE_ALISES)
        {
            ProcessLegacyRichSingleAliases(node, builder);
        }

        if (version <= LAST_VERSION_WITH_LEGACY_STATIC_TEXT)
        {
            ProcessLegacyStaticText(controlReflectedType, node, builder);
        }
    }

    // load children
    const YamlNode* childrenNode = node->Get("children");
    if (childrenNode)
    {
        uint32 count = childrenNode->GetCount();
        for (uint32 i = 0; i < count; i++)
            LoadControl(childrenNode->Get(i), AbstractUIPackageBuilder::TO_PREVIOUS_CONTROL, builder);
    }

    builder->EndControl(controlPlace);
}

void UIPackageLoader::LoadControlPropertiesFromYamlNode(const ReflectedType* ref, const YamlNode* node, AbstractUIPackageBuilder* builder)
{
    static const FastName componentsName("components");

    const TypeInheritance* inheritance = ref->GetType()->GetInheritance();
    if (nullptr != inheritance)
    {
        const Vector<TypeInheritance::Info>& baseTypesInfo = inheritance->GetBaseTypes();
        for (auto& baseInfo : baseTypesInfo)
        {
            LoadControlPropertiesFromYamlNode(ReflectedTypeDB::GetByType(baseInfo.type), node, builder);
        }
    }

    if (ref->GetStructure())
    {
        static FastName componentsName("components");

        const Vector<std::unique_ptr<ReflectedStructure::Field>>& fields = ref->GetStructure()->fields;
        for (const std::unique_ptr<ReflectedStructure::Field>& field : fields)
        {
            if (field->name == componentsName)
            {
                // TODO: Make loading components by reflection here
                continue;
            }

            Any res;
            if (node)
            {
                res = ReadAnyFromYamlNode(field.get(), node, field->name.c_str());
                if (!res.IsEmpty())
                {
                    builder->BeginControlPropertiesSection(ref->GetPermanentName());
                    builder->ProcessProperty(*field, res);
                    builder->EndControlPropertiesSection();
                }
            }
        }
    }
}

void UIPackageLoader::LoadComponentPropertiesFromYamlNode(const YamlNode* node, AbstractUIPackageBuilder* builder)
{
    Vector<ComponentNode> components = ExtractComponentNodes(node);
    for (ComponentNode& nodeDescr : components)
    {
        const ReflectedType* componentRef = builder->BeginComponentPropertiesSection(nodeDescr.type, nodeDescr.index);
        if (componentRef != nullptr && componentRef->GetStructure() != nullptr)
        {
            const Vector<std::unique_ptr<ReflectedStructure::Field>>& fields = componentRef->GetStructure()->fields;
            for (const std::unique_ptr<ReflectedStructure::Field>& field : fields)
            {
                Any res;
                if (nodeDescr.type == Type::Instance<UILinearLayoutComponent>() && version <= LAST_VERSION_WITH_LINEAR_LAYOUT_LEGACY_ORIENTATION)
                {
                    static const FastName orientationName("orientation");
                    FastName name(field->name);
                    if (nodeDescr.type == Type::Instance<UILinearLayoutComponent>() && name == orientationName)
                    {
                        const YamlNode* valueNode = nodeDescr.node->Get(name.c_str());
                        if (valueNode)
                        {
                            if (valueNode->AsString() == "Horizontal")
                            {
                                res = UILinearLayoutComponent::LEFT_TO_RIGHT;
                            }
                            else if (valueNode->AsString() == "Vertical")
                            {
                                res = UILinearLayoutComponent::TOP_DOWN;
                            }
                            else
                            {
                                DVASSERT(false);
                            }
                        }
                    }
                }
                if (nodeDescr.type == Type::Instance<UIControlBackground>() && version <= LAST_VERSION_WITH_LEGACY_SPRITE_MODIFICATION)
                {
                    static const FastName propertyName("spriteModification");
                    const FastName name(field->name);
                    if (name == propertyName)
                    {
                        const YamlNode* valueNode = nodeDescr.node->Get(name.c_str());
                        if (valueNode)
                        {
                            res = valueNode->AsInt32();
                        }
                    }
                }

                if (res.IsEmpty())
                {
                    res = ReadAnyFromYamlNode(field.get(), nodeDescr.node, field->name.c_str());
                }

                builder->ProcessProperty(*field, res);
            }
        }

        builder->EndComponentPropertiesSection();
    }
}

void UIPackageLoader::LoadBindingsFromYamlNode(const YamlNode* node, AbstractUIPackageBuilder* builder)
{
    const YamlNode* bindingsNode = node ? node->Get("bindings") : nullptr;

    if (bindingsNode)
    {
        for (uint32 i = 0; i < bindingsNode->GetCount(); i++)
        {
            const YamlNode* bindingNode = bindingsNode->Get(i);
            if (bindingNode->GetCount() == 3)
            {
                String fieldName = bindingNode->Get(0)->AsString();
                String expression = bindingNode->Get(1)->AsString();
                String modeStr = bindingNode->Get(2)->AsString();
                int32 mode = 0;

                if (GlobalEnumMap<UIDataBindingComponent::UpdateMode>::Instance()->ToValue(modeStr.c_str(), mode))
                {
                    builder->ProcessDataBinding(fieldName, expression, mode);
                }
                else
                {
                    Logger::Error("Unknown binding mode: %s", modeStr.c_str());
                    DVASSERT(false);
                }
            }
            else
            {
                DVASSERT(false);
            }
        }
    }
}

void UIPackageLoader::ProcessLegacyAligns(const YamlNode* node, AbstractUIPackageBuilder* builder) const
{
    bool hasAnchorProperties = false;
    for (const auto& it : legacyAlignsMap)
    {
        if (node->Get(it.second.c_str()))
        {
            hasAnchorProperties = true;
            break;
        }
    }

    if (hasAnchorProperties)
    {
        const ReflectedType* componentRef = builder->BeginComponentPropertiesSection(Type::Instance<UIAnchorComponent>(), 0);
        if (componentRef != nullptr && componentRef->GetStructure() != nullptr)
        {
            const Vector<std::unique_ptr<ReflectedStructure::Field>>& fields = componentRef->GetStructure()->fields;
            for (const std::unique_ptr<ReflectedStructure::Field>& field : fields)
            {
                String name = field->name.c_str();
                auto iter = legacyAlignsMap.find(name);
                if (iter != legacyAlignsMap.end())
                {
                    Any res = ReadAnyFromYamlNode(field.get(), node, iter->second);
                    builder->ProcessProperty(*field, res);
                }
            }
        }

        builder->EndComponentPropertiesSection();
    }
}

void UIPackageLoader::ProcessLegacyDebugDraw(const YamlNode* node, AbstractUIPackageBuilder* builder) const
{
    bool hasDebugDrawProperties = false;
    for (const auto& it : legacyDebugDrawMap)
    {
        if (node->Get(it.second))
        {
            hasDebugDrawProperties = true;
            break;
        }
    }

    if (hasDebugDrawProperties)
    {
        const ReflectedType* componentRef = builder->BeginComponentPropertiesSection(Type::Instance<UIDebugRenderComponent>(), 0);
        if (componentRef != nullptr && componentRef->GetStructure() != nullptr)
        {
            const Vector<std::unique_ptr<ReflectedStructure::Field>>& fields = componentRef->GetStructure()->fields;
            for (const std::unique_ptr<ReflectedStructure::Field>& field : fields)
            {
                static const FastName enabledFieldName("enabled");
                String name(field->name.c_str());
                auto it = legacyDebugDrawMap.find(name);
                if (it != legacyDebugDrawMap.end())
                {
                    Any res = ReadAnyFromYamlNode(field.get(), node, it->second);
                    if (res.IsEmpty() && enabledFieldName == field->name)
                    {
                        res = Any(false);
                    }
                    builder->ProcessProperty(*field, res);
                }
            }
        }

        builder->EndComponentPropertiesSection();
    }
}

void UIPackageLoader::ProcessLegacyClipContent(const YamlNode* node, AbstractUIPackageBuilder* builder) const
{
    if (node->Get("clip"))
    {
        const ReflectedType* componentRef = builder->BeginComponentPropertiesSection(Type::Instance<UIClipContentComponent>(), 0);
        if (componentRef != nullptr && componentRef->GetStructure() != nullptr)
        {
            const Vector<std::unique_ptr<ReflectedStructure::Field>>& fields = componentRef->GetStructure()->fields;
            for (const std::unique_ptr<ReflectedStructure::Field>& field : fields)
            {
                static const FastName enabledFieldName("enabled");
                if (field->name == enabledFieldName)
                {
                    Any res = ReadAnyFromYamlNode(field.get(), node, "clip");
                    if (!res.IsEmpty())
                    {
                        builder->ProcessProperty(*field, res);
                    }
                    break;
                }
            }
        }

        builder->EndComponentPropertiesSection();
    }
}

void UIPackageLoader::ProcessLegacyRichSingleAliases(const YamlNode* node, AbstractUIPackageBuilder* builder) const
{
    static const String COMPONENTS_NAME = "components";
    static const String RICH_CONTENT_NAME = "RichContent";
    static const String ALIASES_NAME = "aliases";

    const YamlNode* richContentWithAliasesNode = nullptr;
    const YamlNode* componentsNode = node->Get(COMPONENTS_NAME);
    if (componentsNode)
    {
        richContentWithAliasesNode = componentsNode->Get(RICH_CONTENT_NAME);
        if (richContentWithAliasesNode && richContentWithAliasesNode->Get(ALIASES_NAME) == nullptr)
        {
            richContentWithAliasesNode = nullptr;
        }
    }

    if (richContentWithAliasesNode != nullptr)
    {
        const ReflectedType* componentRef = builder->BeginComponentPropertiesSection(Type::Instance<UIRichContentAliasesComponent>(), 0);
        if (componentRef != nullptr && componentRef->GetStructure() != nullptr)
        {
            const Vector<std::unique_ptr<ReflectedStructure::Field>>& fields = componentRef->GetStructure()->fields;
            for (const std::unique_ptr<ReflectedStructure::Field>& field : fields)
            {
                String name(field->name.c_str());
                if (name == ALIASES_NAME)
                {
                    Any res = ReadAnyFromYamlNode(field.get(), richContentWithAliasesNode, ALIASES_NAME);
                    if (res.IsEmpty())
                    {
                        res = Any(false);
                    }
                    builder->ProcessProperty(*field, res);
                }
            }
        }

        builder->EndComponentPropertiesSection();
    }
}

void UIPackageLoader::ProcessLegacyStaticText(const ReflectedType* ref, const YamlNode* node, AbstractUIPackageBuilder* builder) const
{
    static const Type* STATIC_TEXT_TYPE = Type::Instance<UIStaticText>();
    // Move properties from UIStaticText to component
    if (ref && ref->GetType() == STATIC_TEXT_TYPE)
    {
        const ReflectedType* componentRef = builder->BeginComponentPropertiesSection(Type::Instance<UITextComponent>(), 0);
        if (componentRef != nullptr && componentRef->GetStructure() != nullptr)
        {
            const Vector<std::unique_ptr<ReflectedStructure::Field>>& fields = componentRef->GetStructure()->fields;
            for (const std::unique_ptr<ReflectedStructure::Field>& field : fields)
            {
                // Find all component properties in UIStaticText yaml node and copy to component
                String name = field->name.c_str();
                auto iter = legacyStaticTextMap.find(name);
                if (iter != legacyStaticTextMap.end())
                {
                    // Copy simple properties
                    Any res = ReadAnyFromYamlNode(field.get(), node, iter->second);
                    builder->ProcessProperty(*field, res);
                }
                else if (name == "fitting")
                {
                    // Convert "fitting" flags
                    const YamlNode* fittingNode = node->Get("fitting");
                    if (fittingNode)
                    {
                        Any res;
                        const auto& fittingsFlags = fittingNode->AsVector();
                        bool enlarge = false, reduce = false, points = false;
                        for (const auto& flag : fittingsFlags)
                        {
                            String flagName = flag->AsString();
                            enlarge = enlarge || (flagName == "ENLARGE");
                            reduce = reduce || (flagName == "REDUCE");
                            points = points || (flagName == "POINTS");
                        }
                        if ((!enlarge) && (!reduce) && (!points))
                        {
                            res = UITextComponent::eTextFitting::FITTING_NONE;
                        }
                        else if (enlarge && reduce && (!points))
                        {
                            res = UITextComponent::eTextFitting::FITTING_FILL;
                        }
                        else if ((!enlarge) && (!reduce) && points)
                        {
                            res = UITextComponent::eTextFitting::FITTING_POINTS;
                        }
                        else if (enlarge && (!reduce) && (!points))
                        {
                            res = UITextComponent::eTextFitting::FITTING_ENLARGE;
                        }
                        else if ((!enlarge) && reduce && (!points))
                        {
                            res = UITextComponent::eTextFitting::FITTING_REDUCE;
                        }
                        else
                        {
                            if (points)
                            {
                                res = UITextComponent::eTextFitting::FITTING_POINTS;
                            }
                            Logger::Warning("[UIPackageLoader::ProcessLegacyStaticText] Invalid fitting combination!");
                        }
                        builder->ProcessProperty(*field, res);
                    }
                }
                else if (name == "multiline")
                {
                    // Convert "multiline" enum
                    const YamlNode* multilineNode = node->Get("multiline");
                    if (multilineNode)
                    {
                        Any res;
                        String multilineString = multilineNode->AsString();
                        if (multilineString == "MULTILINE_ENABLED")
                        {
                            res = UITextComponent::eTextMultiline::MULTILINE_ENABLED;
                        }
                        else if (multilineString == "MULTILINE_ENABLED_BY_SYMBOL")
                        {
                            res = UITextComponent::eTextMultiline::MULTILINE_ENABLED_BY_SYMBOL;
                        }
                        else
                        {
                            res = UITextComponent::eTextMultiline::MULTILINE_DISABLED;
                        }
                        builder->ProcessProperty(*field, res);
                    }
                }
            }
        }
        builder->EndComponentPropertiesSection();
    }
}

Vector<UIPackageLoader::ComponentNode> UIPackageLoader::ExtractComponentNodes(const YamlNode* node)
{
    const YamlNode* componentsNode = node ? node->Get("components") : nullptr;

    Vector<ComponentNode> components;

    if (componentsNode)
    {
        ComponentManager* cm = GetEngineContext()->componentManager;
        for (uint32 i = 0; i < componentsNode->GetCount(); i++)
        {
            const String& fullName = componentsNode->GetItemKeyName(i);
            String::size_type lastChar = fullName.find_last_not_of("0123456789");
            String componentName = fullName.substr(0, lastChar + 1);
            uint32 componentIndex = atoi(fullName.substr(lastChar + 1).c_str());

            const ReflectedType* reflectedType = ReflectedTypeDB::GetByPermanentName(componentName);
            if (reflectedType && cm->IsRegisteredUIComponent(reflectedType->GetType()))
            {
                ComponentNode n;
                n.node = componentsNode->Get(i);
                n.type = reflectedType->GetType();
                n.index = componentIndex;
                components.push_back(n);
            }
            else
            {
                Logger::Error("UIPackageLoader::ExtractComponentNodes UIComponent %s was not registered.", componentName.c_str());
            }
        }

        std::stable_sort(components.begin(), components.end(), [](const ComponentNode& l, const ComponentNode& r) {
            return l.type == r.type ? l.index < r.index : l.type < r.type;
        });
    }
    return components;
}

Any UIPackageLoader::ReadAnyFromYamlNode(const ReflectedStructure::Field* fieldRef, const YamlNode* node, const String& name) const
{
    Any res;
    if (!name.empty())
    {
        const YamlNode* valueNode = node->Get(name);

        if (valueNode)
        {
            res = valueNode->AsAny(fieldRef);
        }
    }

    return res;
}
}

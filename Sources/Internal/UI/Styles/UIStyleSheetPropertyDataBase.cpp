#include "UI/Styles/UIStyleSheetPropertyDataBase.h"
#include "UI/UIControl.h"
#include "UI/UIStaticText.h"
#include "UI/UITextField.h"
#include "UI/UIParticles.h"
#include "UI/UIControlBackground.h"
#include "UI/Layouts/UILinearLayoutComponent.h"
#include "UI/Layouts/UIFlowLayoutComponent.h"
#include "UI/Layouts/UIFlowLayoutHintComponent.h"
#include "UI/Layouts/UIIgnoreLayoutComponent.h"
#include "UI/Layouts/UISizePolicyComponent.h"
#include "UI/Layouts/UIAnchorComponent.h"
#include "UI/Layouts/UIAnchorSafeAreaComponent.h"
#include "UI/Sound/UISoundComponent.h"
#include "UI/Text/UITextComponent.h"
#include "Reflection/ReflectedTypeDB.h"

namespace DAVA
{
UIStyleSheetPropertyDataBase::UIStyleSheetPropertyDataBase()
    : controlGroup("", nullptr, ReflectedTypeDB::Get<UIControl>())
    , bgGroup("bg", Type::Instance<UIControlBackground>(), ReflectedTypeDB::Get<UIControlBackground>())
    , staticTextGroup("text", Type::Instance<UITextComponent>(), ReflectedTypeDB::Get<UITextComponent>())
    , textFieldGroup("textField", nullptr, ReflectedTypeDB::Get<UITextField>())
    , particleEffectGroup("particleEffect", nullptr, ReflectedTypeDB::Get<UIParticles>())
    , linearLayoutGroup("linearLayout", Type::Instance<UILinearLayoutComponent>(), ReflectedTypeDB::Get<UILinearLayoutComponent>())
    , flowLayoutGroup("flowLayout", Type::Instance<UIFlowLayoutComponent>(), ReflectedTypeDB::Get<UIFlowLayoutComponent>())
    , flowLayoutHintGroup("flowLayoutHint", Type::Instance<UIFlowLayoutHintComponent>(), ReflectedTypeDB::Get<UIFlowLayoutHintComponent>())
    , ignoreLayoutGroup("ignoreLayout", Type::Instance<UIIgnoreLayoutComponent>(), ReflectedTypeDB::Get<UIIgnoreLayoutComponent>())
    , sizePolicyGroup("sizePolicy", Type::Instance<UISizePolicyComponent>(), ReflectedTypeDB::Get<UISizePolicyComponent>())
    , anchorGroup("anchor", Type::Instance<UIAnchorComponent>(), ReflectedTypeDB::Get<UIAnchorComponent>())
    , anchorSafeAreaGroup("anchorSafeArea", Type::Instance<UIAnchorSafeAreaComponent>(), ReflectedTypeDB::Get<UIAnchorSafeAreaComponent>())
    , soundGroup("sound", Type::Instance<UISoundComponent>(), ReflectedTypeDB::Get<UISoundComponent>())
    , properties({ { UIStyleSheetPropertyDescriptor(&controlGroup, "angle", 0.0f),
                     UIStyleSheetPropertyDescriptor(&controlGroup, "scale", Vector2(1.0f, 1.0f)),
                     UIStyleSheetPropertyDescriptor(&controlGroup, "pivot", Vector2(0.0f, 0.0f)),
                     UIStyleSheetPropertyDescriptor(&controlGroup, "visible", true),
                     UIStyleSheetPropertyDescriptor(&controlGroup, "noInput", false),
                     UIStyleSheetPropertyDescriptor(&controlGroup, "exclusiveInput", false),

                     UIStyleSheetPropertyDescriptor(&bgGroup, "drawType", static_cast<int32>(UIControlBackground::DRAW_ALIGNED)),
                     UIStyleSheetPropertyDescriptor(&bgGroup, "sprite", FilePath()),
                     UIStyleSheetPropertyDescriptor(&bgGroup, "frame", 0),
                     UIStyleSheetPropertyDescriptor(&bgGroup, "mask", FilePath()),
                     UIStyleSheetPropertyDescriptor(&bgGroup, "detail", FilePath()),
                     UIStyleSheetPropertyDescriptor(&bgGroup, "gradient", FilePath()),
                     UIStyleSheetPropertyDescriptor(&bgGroup, "contour", FilePath()),
                     UIStyleSheetPropertyDescriptor(&bgGroup, "gradientMode", eGradientBlendMode::GRADIENT_MULTIPLY),
                     UIStyleSheetPropertyDescriptor(&bgGroup, "spriteModification", 0),
                     UIStyleSheetPropertyDescriptor(&bgGroup, "color", Color::White),
                     UIStyleSheetPropertyDescriptor(&bgGroup, "colorInherit", UIControlBackground::COLOR_IGNORE_PARENT),
                     UIStyleSheetPropertyDescriptor(&bgGroup, "align", ALIGN_HCENTER | ALIGN_VCENTER),
                     UIStyleSheetPropertyDescriptor(&bgGroup, "leftRightStretchCap", 0.0f),
                     UIStyleSheetPropertyDescriptor(&bgGroup, "topBottomStretchCap", 0.0f),

                     UIStyleSheetPropertyDescriptor(&staticTextGroup, "fontName", String("")),
                     UIStyleSheetPropertyDescriptor(&staticTextGroup, "fontPath", FilePath()),
                     UIStyleSheetPropertyDescriptor(&staticTextGroup, "fontSize", 0.0f),
                     UIStyleSheetPropertyDescriptor(&staticTextGroup, "color", Color::White),
                     UIStyleSheetPropertyDescriptor(&staticTextGroup, "colorInheritType", UIControlBackground::COLOR_MULTIPLY_ON_PARENT),
                     UIStyleSheetPropertyDescriptor(&staticTextGroup, "shadowOffset", Vector2(0.0f, 0.0f)),
                     UIStyleSheetPropertyDescriptor(&staticTextGroup, "shadowColor", Color::White),
                     UIStyleSheetPropertyDescriptor(&staticTextGroup, "align", ALIGN_HCENTER | ALIGN_VCENTER),
                     UIStyleSheetPropertyDescriptor(&staticTextGroup, "multiline", UITextComponent::MULTILINE_DISABLED),

                     UIStyleSheetPropertyDescriptor(&particleEffectGroup, "effectPath", FilePath()),
                     UIStyleSheetPropertyDescriptor(&particleEffectGroup, "autoStart", false),
                     UIStyleSheetPropertyDescriptor(&particleEffectGroup, "startDelay", 0.0f),

                     UIStyleSheetPropertyDescriptor(&linearLayoutGroup, "enabled", true),
                     UIStyleSheetPropertyDescriptor(&linearLayoutGroup, "orientation", UILinearLayoutComponent::LEFT_TO_RIGHT),
                     UIStyleSheetPropertyDescriptor(&linearLayoutGroup, "padding", 0.0f),
                     UIStyleSheetPropertyDescriptor(&linearLayoutGroup, "dynamicPadding", false),
                     UIStyleSheetPropertyDescriptor(&linearLayoutGroup, "spacing", 0.0f),
                     UIStyleSheetPropertyDescriptor(&linearLayoutGroup, "dynamicSpacing", false),

                     UIStyleSheetPropertyDescriptor(&flowLayoutGroup, "enabled", true),
                     UIStyleSheetPropertyDescriptor(&flowLayoutGroup, "orientation", UIFlowLayoutComponent::ORIENTATION_LEFT_TO_RIGHT),
                     UIStyleSheetPropertyDescriptor(&flowLayoutGroup, "hPadding", 0.0f),
                     UIStyleSheetPropertyDescriptor(&flowLayoutGroup, "hDynamicPadding", false),
                     UIStyleSheetPropertyDescriptor(&flowLayoutGroup, "hDynamicInLinePadding", false),
                     UIStyleSheetPropertyDescriptor(&flowLayoutGroup, "hSpacing", 0.0f),
                     UIStyleSheetPropertyDescriptor(&flowLayoutGroup, "hDynamicSpacing", false),
                     UIStyleSheetPropertyDescriptor(&flowLayoutGroup, "vPadding", 0.0f),
                     UIStyleSheetPropertyDescriptor(&flowLayoutGroup, "vDynamicPadding", false),
                     UIStyleSheetPropertyDescriptor(&flowLayoutGroup, "vSpacing", 0.0f),
                     UIStyleSheetPropertyDescriptor(&flowLayoutGroup, "vDynamicSpacing", false),

                     UIStyleSheetPropertyDescriptor(&flowLayoutHintGroup, "newLineBeforeThis", false),
                     UIStyleSheetPropertyDescriptor(&flowLayoutHintGroup, "newLineAfterThis", false),

                     UIStyleSheetPropertyDescriptor(&ignoreLayoutGroup, "enabled", true),

                     UIStyleSheetPropertyDescriptor(&sizePolicyGroup, "horizontalPolicy", UISizePolicyComponent::IGNORE_SIZE),
                     UIStyleSheetPropertyDescriptor(&sizePolicyGroup, "horizontalValue", 100.0f),
                     UIStyleSheetPropertyDescriptor(&sizePolicyGroup, "horizontalMin", 0.0f),
                     UIStyleSheetPropertyDescriptor(&sizePolicyGroup, "horizontalMax", 99999.0f),
                     UIStyleSheetPropertyDescriptor(&sizePolicyGroup, "verticalPolicy", UISizePolicyComponent::IGNORE_SIZE),
                     UIStyleSheetPropertyDescriptor(&sizePolicyGroup, "verticalValue", 100.0f),
                     UIStyleSheetPropertyDescriptor(&sizePolicyGroup, "verticalMin", 0.0f),
                     UIStyleSheetPropertyDescriptor(&sizePolicyGroup, "verticalMax", 99999.0f),

                     UIStyleSheetPropertyDescriptor(&anchorGroup, "enabled", true),
                     UIStyleSheetPropertyDescriptor(&anchorGroup, "leftAnchorEnabled", false),
                     UIStyleSheetPropertyDescriptor(&anchorGroup, "leftAnchor", 0.0f),
                     UIStyleSheetPropertyDescriptor(&anchorGroup, "rightAnchorEnabled", false),
                     UIStyleSheetPropertyDescriptor(&anchorGroup, "rightAnchor", 0.0f),
                     UIStyleSheetPropertyDescriptor(&anchorGroup, "bottomAnchorEnabled", false),
                     UIStyleSheetPropertyDescriptor(&anchorGroup, "bottomAnchor", 0.0f),
                     UIStyleSheetPropertyDescriptor(&anchorGroup, "topAnchorEnabled", false),
                     UIStyleSheetPropertyDescriptor(&anchorGroup, "topAnchor", 0.0f),
                     UIStyleSheetPropertyDescriptor(&anchorGroup, "hCenterAnchorEnabled", false),
                     UIStyleSheetPropertyDescriptor(&anchorGroup, "hCenterAnchor", 0.0f),
                     UIStyleSheetPropertyDescriptor(&anchorGroup, "vCenterAnchorEnabled", false),
                     UIStyleSheetPropertyDescriptor(&anchorGroup, "vCenterAnchor", 0.0f),

                     UIStyleSheetPropertyDescriptor(&anchorSafeAreaGroup, "leftSafeInset", UIAnchorSafeAreaComponent::eInsetType::NONE),
                     UIStyleSheetPropertyDescriptor(&anchorSafeAreaGroup, "leftInsetCorrection", 0.0f),
                     UIStyleSheetPropertyDescriptor(&anchorSafeAreaGroup, "topSafeInset", UIAnchorSafeAreaComponent::eInsetType::NONE),
                     UIStyleSheetPropertyDescriptor(&anchorSafeAreaGroup, "topInsetCorrection", 0.0f),
                     UIStyleSheetPropertyDescriptor(&anchorSafeAreaGroup, "rightSafeInset", UIAnchorSafeAreaComponent::eInsetType::NONE),
                     UIStyleSheetPropertyDescriptor(&anchorSafeAreaGroup, "rightInsetCorrection", 0.0f),
                     UIStyleSheetPropertyDescriptor(&anchorSafeAreaGroup, "bottomSafeInset", UIAnchorSafeAreaComponent::eInsetType::NONE),
                     UIStyleSheetPropertyDescriptor(&anchorSafeAreaGroup, "bottomInsetCorrection", 0.0f),

                     UIStyleSheetPropertyDescriptor(&soundGroup, "touchDown", FastName()),
                     UIStyleSheetPropertyDescriptor(&soundGroup, "touchUpInside", FastName()),
                     UIStyleSheetPropertyDescriptor(&soundGroup, "touchUpOutside", FastName()),
                     UIStyleSheetPropertyDescriptor(&soundGroup, "valueChanged", FastName()) } })
{
    // NewName -> OldNames
    UnorderedMap<FastName, List<FastName>> legacyNames;
    legacyNames[FastName("bg-drawType")] = { FastName("drawType") };
    legacyNames[FastName("bg-sprite")] = { FastName("sprite") };
    legacyNames[FastName("bg-frame")] = { FastName("frame") };
    legacyNames[FastName("bg-color")] = { FastName("color") };
    legacyNames[FastName("bg-colorInherit")] = { FastName("colorInherit") };
    legacyNames[FastName("bg-align")] = { FastName("align") };
    legacyNames[FastName("bg-leftRightStretchCap")] = { FastName("leftRightStretchCap") };
    legacyNames[FastName("bg-topBottomStretchCap")] = { FastName("topBottomStretchCap") };

    legacyNames[FastName("text-fontName")] = { FastName("font"), FastName("text-font") };
    legacyNames[FastName("text-color")] = { FastName("textColor"), FastName("text-textColor") };
    legacyNames[FastName("text-colorInheritType")] = { FastName("textcolorInheritType"), FastName("text-textcolorInheritType") };
    legacyNames[FastName("text-shadowOffset")] = { FastName("shadowoffset"), FastName("text-shadowoffset") };
    legacyNames[FastName("text-shadowColor")] = { FastName("shadowcolor"), FastName("text-shadowcolor") };
    legacyNames[FastName("text-align")] = { FastName("textalign"), FastName("text-textalign") };

    legacyNames[FastName("anchor-leftAnchorEnabled")] = { FastName("leftAnchorEnabled") };
    legacyNames[FastName("anchor-leftAnchor")] = { FastName("leftAnchor") };
    legacyNames[FastName("anchor-rightAnchorEnabled")] = { FastName("rightAnchorEnabled") };
    legacyNames[FastName("anchor-rightAnchor")] = { FastName("rightAnchor") };
    legacyNames[FastName("anchor-bottomAnchorEnabled")] = { FastName("bottomAnchorEnabled") };
    legacyNames[FastName("anchor-bottomAnchor")] = { FastName("bottomAnchor") };
    legacyNames[FastName("anchor-topAnchorEnabled")] = { FastName("topAnchorEnabled") };
    legacyNames[FastName("anchor-topAnchor")] = { FastName("topAnchor") };
    legacyNames[FastName("anchor-hCenterAnchorEnabled")] = { FastName("hCenterAnchorEnabled") };
    legacyNames[FastName("anchor-hCenterAnchor")] = { FastName("hCenterAnchor") };
    legacyNames[FastName("anchor-vCenterAnchorEnabled")] = { FastName("vCenterAnchorEnabled") };
    legacyNames[FastName("anchor-vCenterAnchor")] = { FastName("vCenterAnchor") };

    for (int32 propertyIndex = 0; propertyIndex < STYLE_SHEET_PROPERTY_COUNT; propertyIndex++)
    {
        UIStyleSheetPropertyDescriptor& descr = properties[propertyIndex];
        FastName fullName = FastName(descr.GetFullName());
        propertyNameToIndexMap[fullName] = propertyIndex;

        auto legacyNameIt = legacyNames.find(fullName);
        if (legacyNameIt != legacyNames.end())
        {
            for (auto legacyName : legacyNameIt->second)
            {
                propertyNameToIndexMap[legacyName] = propertyIndex;
            }
        }
    }

    visiblePropertyIndex = GetStyleSheetPropertyIndex(FastName("visible"));
}

UIStyleSheetPropertyDataBase::~UIStyleSheetPropertyDataBase()
{
}

uint32 UIStyleSheetPropertyDataBase::GetStyleSheetPropertyIndex(const FastName& name) const
{
    const auto& iter = propertyNameToIndexMap.find(name);

    DVASSERT(iter != propertyNameToIndexMap.end());

    return iter->second;
}

bool UIStyleSheetPropertyDataBase::IsValidStyleSheetProperty(const FastName& name) const
{
    return propertyNameToIndexMap.find(name) != propertyNameToIndexMap.end();
}

const UIStyleSheetPropertyDescriptor& UIStyleSheetPropertyDataBase::GetStyleSheetPropertyByIndex(uint32 index) const
{
    return properties[index];
}

int32 UIStyleSheetPropertyDataBase::FindStyleSheetProperty(const Type* componentType, const FastName& name) const
{
    for (size_t index = 0; index < properties.size(); index++)
    {
        const UIStyleSheetPropertyDescriptor& descr = properties[index];
        if (descr.group->componentType == componentType && descr.name == name)
        {
            return static_cast<int32>(index);
        }
    }
    return -1;
}
}

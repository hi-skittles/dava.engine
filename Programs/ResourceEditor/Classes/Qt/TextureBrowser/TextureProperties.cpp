#include "Classes/Qt/TextureBrowser/TextureProperties.h"

#include <QtTools/Updaters/LazyUpdater.h>

#include <Base/GlobalEnum.h>
#include <Render/PixelFormatDescriptor.h>
#include <Render/TextureDescriptor.h>

namespace PropertyItemName
{
const DAVA::FastName GenerateMipMaps("generateMipMaps");
const DAVA::FastName IsNormalMap("isNormalMap");
const DAVA::FastName WrapModeS("wrapModeS");
const DAVA::FastName WrapModeT("wrapModeT");
const DAVA::FastName MinFilter("minFilter");
const DAVA::FastName MagFilter("magFilter");
const DAVA::FastName MipFilter("mipFilter");
const DAVA::FastName Format("format");
}

TextureProperties::TextureProperties(QWidget* parent /*= 0*/)
    : QtPropertyEditor(parent)
    , curTextureDescriptor(nullptr)
    , curGPU(DAVA::GPU_ORIGIN)
    , skipPropSizeChanged(false)
{
    SetEditTracking(true);

    DAVA::Function<void()> fn(this, &TextureProperties::ReloadProperties);
    updater = new LazyUpdater(fn, this);

    connect(this, &TextureProperties::PropertyChanged, this, &TextureProperties::OnPropertyChanged);
}

TextureProperties::~TextureProperties()
{
    curTextureDescriptor = nullptr;
}

void TextureProperties::setTextureDescriptor(DAVA::TextureDescriptor* descriptor)
{
    curTextureDescriptor = descriptor;
    origImageSize = QSize(0, 0);

    if (nullptr != curTextureDescriptor)
    {
        // enable this widget
        setEnabled(true);

        // reset mipmap sizes
        // we don't know avaliable mipmap sizes for newly set texture until setOriginalSize() method will be called by user
        MipMapSizesReset();

        // reload all properties for current gpu and from current descriptor
        ReloadProperties();
    }
    else
    {
        // no texture - disable this widget
        setEnabled(false);
        RemovePropertyAll();
    }
}

void TextureProperties::setTextureGPU(DAVA::eGPUFamily gpu)
{
    if (curGPU != gpu)
    {
        curGPU = gpu;
        ReloadProperties();
    }
}

void TextureProperties::setOriginalImageSize(const QSize& size)
{
    origImageSize = size;

    // Init mipmap sizes based on original image size
    MipMapSizesInit(size.width(), size.height());
    updater->Update();
}

const DAVA::TextureDescriptor* TextureProperties::getTextureDescriptor()
{
    return curTextureDescriptor;
}

void TextureProperties::Save()
{
    if (nullptr != curTextureDescriptor)
    {
        curTextureDescriptor->Save();
    }
}

void TextureProperties::MipMapSizesInit(int baseWidth, int baseHeight)
{
    auto RegisterMipLevelSize = [&](int mipLevel, int mipWidth, int mipHeight)
    {
        QSize size(mipWidth, mipHeight);
        QString shownKey;

        if (0 == mipLevel)
        {
            size = QSize(0, 0);
            shownKey = "Original";
        }
        else
        {
            shownKey.sprintf("%dx%d", mipWidth, mipHeight);
        }

        enumSizes.Register(mipLevel, shownKey.toLatin1());
        availableSizes[mipLevel] = size;
    };

    int level = 0;
    MipMapSizesReset();
    while (static_cast<DAVA::uint32>(baseWidth) >= DAVA::Texture::MINIMAL_WIDTH && static_cast<DAVA::uint32>(baseHeight) >= DAVA::Texture::MINIMAL_HEIGHT)
    {
        RegisterMipLevelSize(level, baseWidth, baseHeight);

        level++;
        baseWidth = baseWidth >> 1;
        baseHeight = baseHeight >> 1;
    }

    const DAVA::uint32& width = curTextureDescriptor->compression[curGPU].compressToWidth;
    const DAVA::uint32& height = curTextureDescriptor->compression[curGPU].compressToHeight;
    if ((width != 0 && height != 0) && (width < DAVA::Texture::MINIMAL_WIDTH || height < DAVA::Texture::MINIMAL_HEIGHT))
    {
        RegisterMipLevelSize(level, width, height);
    }

    if (enumSizes.GetCount() > 0)
    {
        LoadCurSizeToProp();
        SetPropertyItemValidValues(propSizes, &enumSizes);
        propSizes->SetEnabled(true);
    }
}

void TextureProperties::MipMapSizesReset()
{
    curSizeLevelObject = 0;
    availableSizes.clear();
    enumSizes.UnregistelAll();
}

void TextureProperties::ReloadProperties()
{
    RemovePropertyAll();

    if (NULL != curTextureDescriptor &&
        curGPU >= 0 &&
        curGPU < DAVA::GPU_DEVICE_COUNT)
    {
        QModelIndex headerIndex;
        DAVA::InspBase* textureDrawSettings = &curTextureDescriptor->drawSettings;
        DAVA::InspBase* textureDataSettings = &curTextureDescriptor->dataSettings;

        // add common texture drawSettings
        headerIndex = AppendHeader("Texture drawSettings");
        propMipMap = AddPropertyItem(PropertyItemName::GenerateMipMaps, textureDataSettings, headerIndex);
        propMipMap->SetCheckable(true);
        propMipMap->SetEditable(false);

        propNormalMap = AddPropertyItem(PropertyItemName::IsNormalMap, textureDataSettings, headerIndex);
        propNormalMap->SetCheckable(true);
        propNormalMap->SetEditable(false);

        //TODO: magic to display introspection info as bool, not int
        bool savedValue = propMipMap->GetValue().toBool();
        propMipMap->SetValue(!savedValue);
        propMipMap->SetValue(savedValue);

        savedValue = propNormalMap->GetValue().toBool();
        propNormalMap->SetValue(!savedValue);
        propNormalMap->SetValue(savedValue);
        //END of TODO

        propWrapModeS = AddPropertyItem(PropertyItemName::WrapModeS, textureDrawSettings, headerIndex);
        propWrapModeT = AddPropertyItem(PropertyItemName::WrapModeT, textureDrawSettings, headerIndex);
        propMinFilter = AddPropertyItem(PropertyItemName::MinFilter, textureDrawSettings, headerIndex);
        propMagFilter = AddPropertyItem(PropertyItemName::MagFilter, textureDrawSettings, headerIndex);
        propMipFilter = AddPropertyItem(PropertyItemName::MipFilter, textureDrawSettings, headerIndex);

        DAVA::InspBase* compressionSettings = &curTextureDescriptor->compression[curGPU];

        // add per-gpu drawSettings
        headerIndex = AppendHeader(GlobalEnumMap<DAVA::eGPUFamily>::Instance()->ToString(curGPU));
        propFormat = AddPropertyItem(PropertyItemName::Format, compressionSettings, headerIndex);

        propSizes = new QtPropertyDataMetaObject(DAVA::FastName("Size"), &curSizeLevelObject, DAVA::MetaInfo::Instance<int>());
        AppendProperty(std::unique_ptr<QtPropertyData>(propSizes), headerIndex);
        LoadCurSizeToProp();

        FinishTreeCreation();
        ReloadEnumFormats();
        ReloadEnumWrap();
        ReloadEnumFilters();

        SetPropertyItemValidValues(propWrapModeS, &enumWpar);
        SetPropertyItemValidValues(propWrapModeT, &enumWpar);
        SetPropertyItemValidValues(propMinFilter, &enumFiltersMin);
        SetPropertyItemValidValues(propMagFilter, &enumFiltersMag);
        SetPropertyItemValidValues(propMipFilter, &enumFiltersMip);
        SetPropertyItemValidValues(propFormat, &enumFormats);
        SetPropertyItemValidValues(propSizes, &enumSizes);

        if (0 == enumSizes.GetCount())
        {
            propSizes->SetEnabled(false);
        }

        expandAll();
    }
}

void TextureProperties::ReloadEnumFormats()
{
    const EnumMap* globalFormats = GlobalEnumMap<DAVA::PixelFormat>::Instance();

    enumFormats.UnregistelAll();

    bool isSquareTexture = origImageSize.width() == origImageSize.height();
    bool isFloatTexture = (curTextureDescriptor != nullptr) && DAVA::PixelFormatDescriptor::IsFloatPixelFormat(curTextureDescriptor->format);

    const DAVA::Map<DAVA::PixelFormat, DAVA::ImageFormat>& availableFormats = DAVA::GPUFamilyDescriptor::GetAvailableFormatsForGpu(curGPU);
    DAVA::PixelFormat currentFormat = curTextureDescriptor->GetPixelFormatForGPU(curGPU);

    for (const auto& nextFormat : availableFormats)
    {
        DAVA::PixelFormat pxFormat = nextFormat.first;
        bool isOldPVR = pxFormat == DAVA::FORMAT_PVR2 || pxFormat == DAVA::FORMAT_PVR4;
        if (!isSquareTexture && isOldPVR && pxFormat != currentFormat)
        {
            // skip PVR2/4 format for non-square textures.
            // but if texture has already had PVR2 or PVR4 compression format we have to show it.
            continue;
        }

        if (isFloatTexture)
        {
            // disable conversion for android
            if ((curGPU == DAVA::GPU_ADRENO) || (curGPU == DAVA::GPU_MALI) || (curGPU == DAVA::GPU_POWERVR_ANDROID) || (curGPU == DAVA::GPU_TEGRA))
                continue;

            // allow converting only float <-> float formats
            if (!DAVA::PixelFormatDescriptor::IsFloatPixelFormat(pxFormat))
                continue;

            // disable non 32f formats for dxt compression (since external tool does not support conversion to 16F)
            if ((pxFormat != DAVA::FORMAT_RGBA32F) && ((curGPU == DAVA::GPU_DX11) || (curGPU == DAVA::GPU_TEGRA) || (curGPU == DAVA::GPU_ADRENO)))
                continue;
        }

        enumFormats.Register(nextFormat.first, globalFormats->ToString(nextFormat.first));
    }
}

void TextureProperties::ReloadEnumFilters()
{
    const EnumMap* filterFormats = GlobalEnumMap<rhi::TextureFilter>::Instance();
    const EnumMap* mipFormats = GlobalEnumMap<rhi::TextureMipFilter>::Instance();

    enumFiltersMag.UnregistelAll();
    enumFiltersMin.UnregistelAll();
    enumFiltersMip.UnregistelAll();

    // Mag
    enumFiltersMag.Register(rhi::TEXFILTER_NEAREST, filterFormats->ToString(rhi::TEXFILTER_NEAREST));
    enumFiltersMag.Register(rhi::TEXFILTER_LINEAR, filterFormats->ToString(rhi::TEXFILTER_LINEAR));

    // Min
    enumFiltersMin.Register(rhi::TEXFILTER_NEAREST, filterFormats->ToString(rhi::TEXFILTER_NEAREST));
    enumFiltersMin.Register(rhi::TEXFILTER_LINEAR, filterFormats->ToString(rhi::TEXFILTER_LINEAR));

    //Mip
    if (nullptr != propMipMap)
    {
        if (propMipMap->GetValue().toBool())
        {
            enumFiltersMip.Register(rhi::TEXMIPFILTER_NONE, mipFormats->ToString(rhi::TEXMIPFILTER_NONE));
            enumFiltersMip.Register(rhi::TEXMIPFILTER_NEAREST, mipFormats->ToString(rhi::TEXMIPFILTER_NEAREST));
            enumFiltersMip.Register(rhi::TEXMIPFILTER_LINEAR, mipFormats->ToString(rhi::TEXMIPFILTER_LINEAR));
        }
        else if (nullptr != propMipFilter)
        {
            enumFiltersMip.Register(rhi::TEXMIPFILTER_NONE, mipFormats->ToString(rhi::TEXMIPFILTER_NONE));

            propMipFilter->SetValue(QVariant(rhi::TEXMIPFILTER_NONE));
        }
    }
}

void TextureProperties::ReloadEnumWrap()
{
    const EnumMap* globalFormats = GlobalEnumMap<rhi::TextureAddrMode>::Instance();

    enumWpar.UnregistelAll();

    enumWpar.Register(rhi::TEXADDR_WRAP, globalFormats->ToString(rhi::TEXADDR_WRAP));
    enumWpar.Register(rhi::TEXADDR_CLAMP, globalFormats->ToString(rhi::TEXADDR_CLAMP));
}

QtPropertyDataInspMember* TextureProperties::AddPropertyItem(const DAVA::FastName& name, DAVA::InspBase* object, const QModelIndex& parent)
{
    QtPropertyDataInspMember* ret = nullptr;
    const DAVA::InspInfo* info = object->GetTypeInfo();

    if (nullptr != info)
    {
        const DAVA::InspMember* member = info->Member(name);
        if (nullptr != member)
        {
            ret = new QtPropertyDataInspMember(member->Name(), object, member);
            AppendProperty(std::unique_ptr<QtPropertyData>(ret), parent);
        }
    }

    return ret;
}

void TextureProperties::SetPropertyItemValidValues(QtPropertyDataInspMember* item, EnumMap* validValues)
{
    if (item == nullptr)
        return;

    DAVA::uint32 valuesCount = 0;
    item->ClearAllowedValues();

    if ((validValues != nullptr) && (validValues->GetCount() > 0))
    {
        for (size_t i = 0; i < validValues->GetCount(); ++i)
        {
            int v = 0;
            if (validValues->GetValue(i, v))
            {
                item->AddAllowedValue(DAVA::VariantType(v), validValues->ToString(v));
                ++valuesCount;
            }
        }
    }

    item->SetEnabled(valuesCount > 0);
}

void TextureProperties::SetPropertyItemValidValues(QtPropertyDataMetaObject* item, EnumMap* validValues)
{
    if (nullptr != item && nullptr != validValues)
    {
        item->ClearAllowedValues();
        for (size_t i = 0; i < validValues->GetCount(); ++i)
        {
            int v;

            if (validValues->GetValue(i, v))
            {
                item->AddAllowedValue(DAVA::VariantType(v), validValues->ToString(v));
            }
        }
    }
}

void TextureProperties::OnItemEdited(const QModelIndex& index)
{
    QtPropertyEditor::OnItemEdited(index);

    QtPropertyData* data = GetProperty(index);
    if (data == propMipMap)
    {
        emit PropertyChanged(PROP_MIPMAP);
    }
    else if (data == propNormalMap)
    {
        emit PropertyChanged(PROP_NORMALMAP);
    }
    else if (data == propFormat)
    {
        emit PropertyChanged(PROP_FORMAT);
    }
    else if (data == propMinFilter || data == propMagFilter || data == propMipFilter)
    {
        emit PropertyChanged(PROP_FILTER);
    }
    else if (data == propWrapModeS || data == propWrapModeT)
    {
        emit PropertyChanged(PROP_WRAP);
    }
    else if (data == propSizes)
    {
        SaveCurSizeFromProp();

        if (!skipPropSizeChanged)
        {
            emit PropertyChanged(PROP_SIZE);
        }

        // re-Init mipmap sizes based on original image size
        MipMapSizesInit(origImageSize.width(), origImageSize.height());
    }

    Save();
}

void TextureProperties::OnPropertyChanged(int type)
{
    if (PROP_MIPMAP == type || PROP_FORMAT == type)
    {
        updater->Update();
    }
}

void TextureProperties::LoadCurSizeToProp()
{
    if (nullptr != curTextureDescriptor && nullptr != propSizes &&
        curGPU >= 0 && curGPU < DAVA::GPU_DEVICE_COUNT)
    {
        QSize curSize(curTextureDescriptor->compression[curGPU].compressToWidth, curTextureDescriptor->compression[curGPU].compressToHeight);
        int level = availableSizes.key(curSize, -1);

        if (-1 != level)
        {
            skipPropSizeChanged = true;
            propSizes->SetValue(level);
            propSizes->UpdateValue(true);
            skipPropSizeChanged = false;
        }
    }
}

void TextureProperties::SaveCurSizeFromProp()
{
    if (nullptr != curTextureDescriptor && nullptr != propSizes &&
        curGPU >= 0 && curGPU < DAVA::GPU_DEVICE_COUNT)
    {
        int level = propSizes->GetValue().toInt();

        if (availableSizes.contains(level))
        {
            DVASSERT(curTextureDescriptor->compression);
            curTextureDescriptor->compression[curGPU].compressToWidth = availableSizes[level].width();
            curTextureDescriptor->compression[curGPU].compressToHeight = availableSizes[level].height();
        }
    }
}

#include "CommandLine/Private/OptionName.h"
#include "Render/GPUFamilyDescriptor.h"

const DAVA::String OptionName::Output("-output");
const DAVA::String OptionName::OutFile("-outfile");
const DAVA::String OptionName::OutDir("-outdir");

const DAVA::String OptionName::ResourceDir("-resdir");

const DAVA::String OptionName::File("-file");
const DAVA::String OptionName::ProcessFile("-processfile");

const DAVA::String OptionName::ProcessFileList("-processfilelist");

const DAVA::String OptionName::Folder("-folder");
const DAVA::String OptionName::InDir("-indir");
const DAVA::String OptionName::ProcessDir("-processdir");

const DAVA::String OptionName::QualityConfig("-qualitycfgpath");

const DAVA::String OptionName::Split("-split");
const DAVA::String OptionName::Merge("-merge");
const DAVA::String OptionName::Save("-save");
const DAVA::String OptionName::Resave("-resave");
const DAVA::String OptionName::Build("-build");
const DAVA::String OptionName::Convert("-convert");
const DAVA::String OptionName::Create("-create");

const DAVA::String OptionName::Links("-links");
const DAVA::String OptionName::Scene("-scene");
const DAVA::String OptionName::Texture("-texture");
const DAVA::String OptionName::Yaml("-yaml");

const DAVA::String OptionName::GPU("-gpu");
const DAVA::String OptionName::Quality("-quality");
const DAVA::String OptionName::Force("-f");
const DAVA::String OptionName::Mipmaps("-m");
const DAVA::String OptionName::HDTextures("-hd");
const DAVA::String OptionName::Mode("-mode");

const DAVA::String OptionName::SaveNormals("-saveNormals");
const DAVA::String OptionName::CopyConverted("-copyconverted");
const DAVA::String OptionName::SetCompression("-setcompression");
const DAVA::String OptionName::SetPreset("-setpreset");
const DAVA::String OptionName::SavePreset("-savepreset");
const DAVA::String OptionName::PresetOpt("-preset");
const DAVA::String OptionName::PresetsList("-presetslist");

const DAVA::String OptionName::UseAssetCache("-useCache");
const DAVA::String OptionName::AssetCacheIP("-ip");
const DAVA::String OptionName::AssetCachePort("-p");
const DAVA::String OptionName::AssetCacheTimeout("-t");

const DAVA::String OptionName::Width("-width");
const DAVA::String OptionName::Height("-height");
const DAVA::String OptionName::Camera("-camera");

const DAVA::String OptionName::Validate("-validate");
const DAVA::String OptionName::Count("-count");

const DAVA::String OptionName::Tag("-tag");
const DAVA::String OptionName::TagList("-taglist");

const DAVA::String OptionName::MakeNameForGPU(DAVA::eGPUFamily gpuFamily)
{
    return ("-" + DAVA::GPUFamilyDescriptor::GetGPUName(gpuFamily));
}

#pragma once

#include "Base/BaseTypes.h"
#include "Render/RenderBase.h"

//command line constants for unification of command line
class OptionName
{
public:
    static const DAVA::String Output;
    static const DAVA::String OutFile;
    static const DAVA::String OutDir;

    static const DAVA::String ResourceDir;

    static const DAVA::String File;
    static const DAVA::String ProcessFile;

    static const DAVA::String ProcessFileList;

    static const DAVA::String Folder;
    static const DAVA::String InDir;
    static const DAVA::String ProcessDir;

    static const DAVA::String QualityConfig;

    static const DAVA::String Split;
    static const DAVA::String Merge;
    static const DAVA::String Save;
    static const DAVA::String Resave;
    static const DAVA::String Build;
    static const DAVA::String Convert;
    static const DAVA::String Create;

    static const DAVA::String Links;
    static const DAVA::String Scene;
    static const DAVA::String Texture;
    static const DAVA::String Yaml;

    static const DAVA::String GPU;
    static const DAVA::String Quality;
    static const DAVA::String Force;
    static const DAVA::String Mipmaps;
    static const DAVA::String HDTextures;
    static const DAVA::String Mode;

    static const DAVA::String SaveNormals;
    static const DAVA::String CopyConverted;
    static const DAVA::String SetCompression;
    static const DAVA::String SetPreset;
    static const DAVA::String SavePreset;
    static const DAVA::String PresetOpt;
    static const DAVA::String PresetsList;

    static const DAVA::String UseAssetCache;
    static const DAVA::String AssetCacheIP;
    static const DAVA::String AssetCachePort;
    static const DAVA::String AssetCacheTimeout;

    static const DAVA::String Width;
    static const DAVA::String Height;
    static const DAVA::String Camera;

    static const DAVA::String Validate;
    static const DAVA::String Count;

    static const DAVA::String Tag;
    static const DAVA::String TagList;

    static const DAVA::String MakeNameForGPU(DAVA::eGPUFamily gpuFamily);
};

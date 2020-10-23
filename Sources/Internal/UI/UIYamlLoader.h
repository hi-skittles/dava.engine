#ifndef __DAVAENGINE_YAML_LOADER_H__
#define __DAVAENGINE_YAML_LOADER_H__

#include "Base/BaseTypes.h"
#include "Base/BaseObject.h"
#include "FileSystem/FilePath.h"
#include "Render/2D/FontManager.h"

namespace DAVA
{
/**
    \ingroup controlsystem
    \brief Class to perform loading of controls from yaml configs

    This class can help you to load hierarhy of controls from yaml config file.
    Structure of yaml file is very simple and it allow you to modify the structure of the application easily using
    configuration files.
*/

class YamlNode;

class UIYamlLoader : public BaseObject
{
protected:
    ~UIYamlLoader() = default;
    UIYamlLoader() = default;

public:
    /**
     \brief	This is main function in UIYamlLoader and it loads fonts from yamlPathname file.

     \param[in] yamlPathName						we get config file using this pathname
     */
    static void LoadFonts(const FilePath& yamlPathname);
    static FontPreset CreateFontPresetFromYamlNode(const YamlNode* node);
    static YamlNode* CreateYamlNodeFromFontPreset(const FontPreset& preset);

    /**
     \brief	This function saves fonts to the YAML file passed.

     \param[in]			yamlPathName	path to store fonts to
     \return            true if the save was successful
     */
    static bool SaveFonts(const FilePath& yamlPathname);

    const FontPreset& GetFontPresetByName(const String& fontName) const;

protected:
    //Internal functions that do actual loading and saving.
    YamlNode* CreateRootNode(const FilePath& yamlPathname);
    void LoadFontsFromNode(const YamlNode* node);
};
};

#endif // __DAVAENGINE_YAML_LOADER_H__

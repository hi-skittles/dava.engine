#ifndef __DAVAENGINE_UI_STYLESHEET_YAML_LOADER_H__
#define __DAVAENGINE_UI_STYLESHEET_YAML_LOADER_H__

#include "Base/BaseTypes.h"
#include "FileSystem/FilePath.h"

namespace DAVA
{
class UIStyleSheet;
class YamlNode;

class UIStyleSheetYamlLoader
{
public:
    UIStyleSheetYamlLoader();

    void LoadFromYaml(const YamlNode* rootNode, Vector<UIStyleSheet*>* styleSheets);
};
};


#endif

#ifndef __DAVAENGINE_YAML_ARCHIVE_H__
#define __DAVAENGINE_YAML_ARCHIVE_H__

#include "FileSystem/YamlParser.h"

namespace DAVA
{
/** 
	\ingroup yaml
	\brief this class is class to load yaml files not only in text format but cache them in binary store also for fast loading
	\todo add this class to optimize work with big yaml configs
 */
class YamlArchive : public BaseObject
{
protected:
    ~YamlArchive()
    {
    }

public:
}
};

#endif // __DAVAENGINE_YAML_ARCHIVE_H__

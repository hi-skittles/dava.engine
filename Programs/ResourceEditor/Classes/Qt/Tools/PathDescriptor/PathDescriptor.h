#ifndef __PATH_DESCRIPTOR_H__
#define __PATH_DESCRIPTOR_H__

#include <QString>
#include "Base/BaseTypes.h"

class PathDescriptor
{
public:
    enum eType
    {
        PATH_TEXTURE = 0,
        PATH_IMAGE,
        PATH_HEIGHTMAP,
        PATH_TEXTURE_SHEET,
        PATH_SCENE,
        PATH_NOT_SPECIFIED
    };

    PathDescriptor(const QString& name, const QString& filter, eType type)
        : pathName(name)
        , fileFilter(filter)
        , pathType(type)
    {
        ;
    };

    QString pathName;
    QString fileFilter;
    eType pathType;

    static void InitializePathDescriptors();
    static DAVA::Vector<PathDescriptor> descriptors;
    static PathDescriptor& GetPathDescriptor(eType type);
};



#endif // __PATH_DESCRIPTOR_H__

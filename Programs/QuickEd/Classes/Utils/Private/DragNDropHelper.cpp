#include "Classes/Utils/DragNDropHelper.h"

#include "Classes/Modules/ProjectModule/ProjectData.h"

#include <TArc/Core/ContextAccessor.h>
#include <TArc/DataProcessing/DataContext.h>

#include <FileSystem/FilePath.h>

#include <QFileInfo>
#include <QString>
#include <QUrl>

namespace DragNDropHelper
{
using namespace DAVA;

bool IsFileFromProject(DAVA::ContextAccessor* accessor, const QString& path)
{
    QFileInfo fileInfo(path);
    if (fileInfo.isFile())
    {
        ProjectData* projectData = accessor->GetGlobalContext()->GetData<ProjectData>();
        if (projectData == nullptr)
        {
            return false;
        }

        const Vector<FilePath> allowedPathes = {
            projectData->GetResourceDirectory().absolute,
            projectData->GetAdditionalResourceDirectory().absolute,
            projectData->GetConvertedResourceDirectory().absolute
        };

        for (const FilePath& allowedPath : allowedPathes)
        {
            if (allowedPath.IsEmpty() == false && path.startsWith(QString::fromStdString(allowedPath.GetAbsolutePathname())))
            {
                return true;
            }
        }
    }
    return false;
}

bool IsExtensionSupported(const QString& path)
{
    return path.endsWith("yaml");
}
}

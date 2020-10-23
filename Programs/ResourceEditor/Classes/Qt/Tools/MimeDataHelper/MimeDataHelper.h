#ifndef __RESOURCEEDITORQT__MIMEDATAHELPER__
#define __RESOURCEEDITORQT__MIMEDATAHELPER__

#include "DAVAEngine.h"
#include <QMimeData>

namespace DAVA
{
class SceneEditor2;
} // namespace DAVA

namespace DAVA
{
class MimeDataHelper
{
public:
    static bool IsMimeDataTypeSupported(const QMimeData* mimeData);

    static bool IsMimeDataTypeSupported(const String& mimeType);

    static void GetItemNamesFromMimeData(const QMimeData* mimeData, List<String>& nameList);

    static void ConvertToMimeData(List<FilePath>&, QMimeData* mimeData);

    static void ConvertToMimeData(List<Entity*>&, QMimeData* mimeData);

    static List<Entity*> GetPointersFromSceneTreeMime(const QMimeData* mimeData);

protected:
    static void GetItemNamesFromSceneTreeMime(const QMimeData* mimeData, List<String>& nameList);

    static void GetItemNamesFromFilePathMime(const QMimeData* mimeData, List<String>& nameList);

    struct MimeHandler
    {
        DAVA::String format;
        void (*getNameFuncPtr)(const QMimeData* mimeData, List<String>& nameList);
        MimeHandler(DAVA::String _format,
                    void (*_getNameFuncPtr)(const QMimeData* mimeData, List<String>& nameList))
        {
            format = _format;
            getNameFuncPtr = _getNameFuncPtr;
        }
    };

    const static MimeHandler mimeHandlerMap[];
};

}; //DAVA nemaspace
#endif /* defined(__RESOURCEEDITORQT__MIMEDATAHELPER__) */
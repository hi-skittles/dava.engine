#ifndef __TEXTURE_CONVERTOR_H__
#define __TEXTURE_CONVERTOR_H__

#include "Base/BaseTypes.h"

#include <QObject>
#include <QFutureWatcher>
#include <QProgressDialog>

#include "DAVAEngine.h"
#include "Render/TextureDescriptor.h"

#include "TextureInfo.h"
#include "TextureConvertorWork.h"
#include "TextureConvertMode.h"

#include "Tools/QtWaitDialog/QtWaitDialog.h"

#define CONVERT_JOB_COUNT 2

class TextureConvertor : public QObject, public DAVA::Singleton<TextureConvertor>
{
    Q_OBJECT

public:
    TextureConvertor();
    ~TextureConvertor();

    static DAVA::Vector<DAVA::Image*> ConvertFormat(DAVA::TextureDescriptor* descriptor, DAVA::eGPUFamily gpu,
                                                    eTextureConvertMode convertMode);

    int GetThumbnail(const DAVA::TextureDescriptor* descriptor);
    int GetOriginal(const DAVA::TextureDescriptor* descriptor);
    int GetConverted(const DAVA::TextureDescriptor* descriptor, DAVA::eGPUFamily gpu,
                     eTextureConvertMode convertMode = CONVERT_NOT_EXISTENT);
    int Reconvert(DAVA::Scene* scene, eTextureConvertMode convertMode);

    void WaitConvertedAll(QWidget* parent);
    void CancelConvert();

signals:
    void ReadyThumbnail(const DAVA::TextureDescriptor* descriptor, const TextureInfo& image);
    void ReadyOriginal(const DAVA::TextureDescriptor* descriptor, const TextureInfo& image);
    void ReadyConverted(const DAVA::TextureDescriptor* descriptor, const DAVA::eGPUFamily gpu, const TextureInfo& image);
    void ReadyReconvert();

    void ReadyConvertedAll();

    void ConvertStatusImg(const QString& imgPath, int imgGpu);
    void ConvertStatusQueue(int curJob, int jobCount);

private:
    int jobIdCounter;
    int convertJobQueueSize;

    bool waitingComletion;
    QString waitStatusText;

    JobStack jobStackThumbnail;
    JobStack jobStackOriginal;
    JobStack jobStackConverted;

    JobWatcher watcherThumbnail;
    JobWatcher watcherOriginal;
    JobWatcher watcherConverted;

    QtWaitDialog* waitDialog = nullptr;

    void jobRunNextConvert();
    void jobRunNextOriginal();
    void jobRunNextThumbnail();

    TextureInfo GetThumbnailThread(const JobItem* item);
    TextureInfo GetOriginalThread(const JobItem* item);
    TextureInfo GetConvertedThread(const JobItem* item);

    void ThreadThumbnailFinished(const TextureInfo& info, const JobItem* item);
    void ThreadOriginalFinished(const TextureInfo& info, const JobItem* item);
    void ThreadConvertedFinished(const TextureInfo& info, const JobItem* item);

private slots:
    void waitCanceled();
};

#endif // __TEXTURE_CONVERTOR_H__

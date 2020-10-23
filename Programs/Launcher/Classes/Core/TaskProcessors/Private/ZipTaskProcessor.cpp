#include "Core/TaskProcessors/ZipTaskProcessor.h"
#include "Core/CommonTasks/UnzipTask.h"

#include <QApplication>
#include <QDir>
#include <QRegularExpression>

ZipTaskProcessor::TaskParams::TaskParams(std::unique_ptr<BaseTask>&& task_, const Notifier& notifier_)
    : task(static_cast<UnzipTask*>(task_.release()))
    , notifier(notifier_)
    , process(new QProcess())
{
    notifier.NotifyStarted(task.get());
}

ZipTaskProcessor::TaskParams::~TaskParams()
{
    notifier.NotifyFinished(task.get());

    if (process.isNull() == false)
    {
        process->disconnect();
        process->deleteLater();
    }
}

ZipTaskProcessor::ZipTaskProcessor(QObject* parent)
    : QObject(parent)
{
}

ZipTaskProcessor::~ZipTaskProcessor()
{
    Q_ASSERT(currentTaskParams == nullptr);
}

void ZipTaskProcessor::AddTask(std::unique_ptr<BaseTask>&& task, Notifier notifier)
{
    Q_ASSERT(task->GetTaskType() == BaseTask::ZIP_TASK);
    Q_ASSERT(currentTaskParams == nullptr);

    currentTaskParams = std::make_unique<TaskParams>(std::move(task), notifier);

    QString processAddr = GetArchiverPath();
    if (QFile::exists(processAddr) == false)
    {
        currentTaskParams->task->SetError(QObject::tr("Archiver tool was not found. Please, reinstall application"));
    }
    else
    {
        QString archivePath = currentTaskParams->task->GetArchivePath();
        if (archivePath.endsWith(".zip") == false)
        {
            currentTaskParams->task->SetError(QObject::tr("Required file is not a zip archive"));
        }
        else if (QFile::exists(archivePath) == false)
        {
            currentTaskParams->task->SetError(QObject::tr("Required archive doesn't exists"));
        }
    }

    if (currentTaskParams->task->HasError())
    {
        currentTaskParams = nullptr;
        return;
    }

    connect(currentTaskParams->process, &QProcess::readyReadStandardOutput, this, &ZipTaskProcessor::OnReadyReadStandardOutput);
    connect(currentTaskParams->process, &QProcess::errorOccurred, this, &ZipTaskProcessor::OnErrorOccurred);
    connect(currentTaskParams->process, static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished), this, &ZipTaskProcessor::OnFinished);

    ApplyState();
}

void ZipTaskProcessor::Terminate()
{
    if (currentTaskParams != nullptr)
    {
        currentTaskParams->task->SetError("Archiver was stopped");

        currentTaskParams = nullptr;
    }
}

void ZipTaskProcessor::OnErrorOccurred(QProcess::ProcessError error)
{
    switch (error)
    {
    case QProcess::FailedToStart:
        currentTaskParams->task->SetError(QObject::tr("Failed to launch archiver"));
        break;
    case QProcess::Crashed:
        currentTaskParams->task->SetError(QObject::tr("Archiver was stopped"));
        break;
    case QProcess::Timedout:
        currentTaskParams->task->SetError(QObject::tr("Archiver time out error"));
        break;
    case QProcess::ReadError:
        currentTaskParams->task->SetError(QObject::tr("Failed to read output from archiver"));
        break;
    case QProcess::WriteError:
        currentTaskParams->task->SetError(QObject::tr("Failed to write to archiver"));
        break;
    case QProcess::UnknownError:
        currentTaskParams->task->SetError(QObject::tr("Archiver crashed with an unknown error"));
        break;
    }
}

void ZipTaskProcessor::OnFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (exitCode != 0 || exitStatus != QProcess::NormalExit)
    {
        if (currentTaskParams->task->HasError() == false)
        {
            currentTaskParams->task->SetError(QObject::tr("Archiver reports about error in current archive"));
        }
    }
    else if (currentTaskParams->state == LIST_ARCHIVE)
    {
        currentTaskParams->totalSize = std::accumulate(currentTaskParams->filesAndSizes.begin(),
                                                       currentTaskParams->filesAndSizes.end(),
                                                       0,
                                                       [](quint64 value, const std::pair<QString, quint64>& pair) {
                                                           return value + pair.second;
                                                       });
        currentTaskParams->state = UNPACK;
        ApplyState();
        return;
    }
    currentTaskParams = nullptr;
}

void ZipTaskProcessor::OnReadyReadStandardOutput()
{
    while (currentTaskParams->process->canReadLine())
    {
        QByteArray line = currentTaskParams->process->readLine();
        if (currentTaskParams->state == LIST_ARCHIVE)
        {
            ProcessOutputForList(line);
        }
        else
        {
            ProcessOutputForUnpack(line);
        }
    }
    qApp->processEvents();
}

void ZipTaskProcessor::ApplyState()
{
    QString processAddr = GetArchiverPath();
    QStringList arguments;
    if (currentTaskParams->state == LIST_ARCHIVE)
    {
        arguments << "l" << currentTaskParams->task->GetArchivePath();
    }
    else
    {
        //this is needed for correct work with pathes contains whitespace
        QString nativeOutPath = QDir::toNativeSeparators(currentTaskParams->task->GetOutputPath());
        arguments << "x"
                  << "-y"
                  << "-bb1"
                  << currentTaskParams->task->GetArchivePath()
                  << "-o" + nativeOutPath;
    }
    currentTaskParams->process->start(processAddr, arguments);
}

void ZipTaskProcessor::ProcessOutputForList(const QByteArray& line)
{
    if (line.startsWith("----------")) //this string occurrs two times: before file list and at the and of file list
    {
        currentTaskParams->foundOutputData = !currentTaskParams->foundOutputData;
        return;
    }
    if (currentTaskParams->foundOutputData == false)
    {
        return;
    }
    QString str(line);

    const int SIZE_INDEX = 26; //fixed index for size
    const int SIZE_STR_LEN = 12; //fixed index for len
    const int NAME_INDEX = 53; //fixed index for name

    bool ok = true;
    QString sizeStr = str.mid(SIZE_INDEX, SIZE_STR_LEN);
    sizeStr.remove(QRegularExpression("\\s*"));
    qint64 size = sizeStr.toULongLong(&ok);
    if (!ok || str.length() <= NAME_INDEX)
    {
        currentTaskParams->task->SetError(QObject::tr("Unknown format of archiver output"));
        return;
    }

    QString file = str.right(str.size() - NAME_INDEX);
    Q_ASSERT(currentTaskParams->filesAndSizes.find(file) == currentTaskParams->filesAndSizes.end());
    currentTaskParams->filesAndSizes[file] = size;
}

void ZipTaskProcessor::ProcessOutputForUnpack(const QByteArray& line)
{
    QString str(line);
    QString startStr("- ");
    if (!str.startsWith(startStr))
    {
        return;
    }
    str.remove(0, startStr.length());
    auto iter = currentTaskParams->filesAndSizes.find(str);
    if (iter != currentTaskParams->filesAndSizes.end())
    {
        currentTaskParams->matchedSize += iter->second;
    }
    float progress = (currentTaskParams->matchedSize * 100.0f) / currentTaskParams->totalSize;
    int progressInt = static_cast<int>(qRound(progress));
    currentTaskParams->notifier.NotifyProgress(currentTaskParams->task.get(), progressInt);
}

QString ZipTaskProcessor::GetArchiverPath() const
{
    static QString processAddr = qApp->applicationDirPath() +
#if defined(Q_OS_WIN)
    "/7z.exe";
#elif defined Q_OS_MAC
    "/../Resources/7za";
#endif //Q_OS_MAC Q_OS_WIN
    return processAddr;
}

std::size_t ZipTaskProcessor::GetTasksCount() const
{
    return currentTaskParams != nullptr ? 1 : 0;
}

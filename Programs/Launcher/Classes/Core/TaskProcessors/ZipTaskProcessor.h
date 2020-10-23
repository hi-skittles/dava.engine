#pragma once

#include "Core/TaskProcessors/BaseTaskProcessor.h"

#include <QPointer>
#include <QProcess>

class UnzipTask;
class QProcess;

class ZipTaskProcessor final : public QObject, public BaseTaskProcessor
{
    Q_OBJECT

public:
    ZipTaskProcessor(QObject* parent);
    ~ZipTaskProcessor();

private slots:
    void OnErrorOccurred(QProcess::ProcessError error);
    void OnFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void OnReadyReadStandardOutput();

private:
    enum eState
    {
        LIST_ARCHIVE,
        UNPACK
    };

    void AddTask(std::unique_ptr<BaseTask>&& task, Notifier notifier) override;
    void Terminate() override;
    std::size_t GetTasksCount() const override;

    void ApplyState();

    void ProcessOutputForList(const QByteArray& line);
    void ProcessOutputForUnpack(const QByteArray& line);

    QString GetArchiverPath() const;

    struct TaskParams
    {
        TaskParams(std::unique_ptr<BaseTask>&& task, const Notifier& notifier);
        ~TaskParams();

        std::unique_ptr<UnzipTask> task;
        Notifier notifier;

        QPointer<QProcess> process;

        eState state = LIST_ARCHIVE;
        bool foundOutputData = false;
        std::map<QString, quint64> filesAndSizes;
        quint64 matchedSize = 0;
        quint64 totalSize = 0;
    };
    std::unique_ptr<TaskParams> currentTaskParams;
};

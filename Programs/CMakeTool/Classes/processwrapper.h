#pragma once

#include <QObject>
#include <QProcess>
#include <QQueue>
#include <QVariant>

class ProcessWrapper : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool running READ IsRunning NOTIFY runningChanged)

public:
    explicit ProcessWrapper(QObject* parent = 0);

    Q_INVOKABLE void StartConfigure(const QString& command, bool needClean, const QString& buildFolder);
    Q_INVOKABLE void LaunchCmake(const QString& command);

    Q_INVOKABLE void FindAndOpenProjectFile(const QString& buildFolder);
    Q_INVOKABLE void OpenFolderInExplorer(const QString& folderPath);
    Q_INVOKABLE void BlockingStopAllTasks();
    Q_INVOKABLE void KillProcess();
    bool IsRunning() const;

signals:
    void configureFinished();
    void configureFailed();

    void configureStarted();
    void configureEnded();

    void processStateTextChanged(const QString& text);
    void processErrorTextChanged(const QString& text);
    void processStandardOutput(const QString& text) const;
    void processStandardError(const QString& text) const;
    void testSignal();
    void runningChanged(bool running);

private slots:
    void OnReadyReadStandardOutput();
    void OnReadyReadStandardError();
    void OnProcessStateChanged(QProcess::ProcessState newState);
    void OnProcessError(QProcess::ProcessError error);

private:
    Q_INVOKABLE void StartNextCommand();
    bool CleanBuildFolder(const QString& buildFolder) const;
    void SetRunning(bool running);

    QProcess process;
    struct Task
    {
        Task(const QString& command_, bool needClean_, const QString& buildFolder_, bool configure_)
            : command(command_)
            , needClean(needClean_)
            , buildFolder(buildFolder_)
            , configure(configure_)
        {
        }
        QString command;
        //flag to clean folder
        bool needClean;
        //and folder to clean
        QString buildFolder;
        //flag that we start configuring project and need result of that configuring
        bool configure;
    };
    QQueue<Task> taskQueue;
    bool running = false;

    struct ProcessDetails
    {
        bool configuringProject = false;
        bool hasErrors = false;
    };
    ProcessDetails currentProcessDetails;
};

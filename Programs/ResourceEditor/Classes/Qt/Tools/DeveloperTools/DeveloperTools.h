#pragma once

#include <QObject>

class DeveloperTools : public QObject
{
    Q_OBJECT

public:
    explicit DeveloperTools(QWidget* parent = 0);

public slots:

    void OnDebugFunctionsGridCopy();
    void OnDebugCreateTestHardSkinnedObject();
    void OnDebugCreateTestSoftSkinnedObject();
    void OnImageSplitterNormals();
    void OnReplaceTextureMipmap();
    void OnToggleLandscapeInstancing();

private:
    QWidget* GetParentWidget();
};

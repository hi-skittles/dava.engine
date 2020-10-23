#include "DebugTools/DebugTools.h"

#include "Render/2D/Sprite.h"
#include "Render/Texture.h"
#include "UI/UIControl.h"

#include <QObject>

namespace DebugTools
{
void ConnectToUI(Ui::MainWindow* ui)
{
    QObject::connect(ui->actionDump_Controls, &QAction::triggered, [] {
        DAVA::UIControl::DumpControls(false);
    });

#if !defined(__DAVAENGINE_DEBUG__)
    ui->actionDump_Controls->setVisible(false);
#endif //#if !defined(__DAVAENGINE_DEBUG__)

    QObject::connect(ui->actionDump_Textures, &QAction::triggered, [] {
        DAVA::Texture::DumpTextures();
    });

    QObject::connect(ui->actionDump_Sprites, &QAction::triggered, [] {
        DAVA::Sprite::DumpSprites();
    });
}
}
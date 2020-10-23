#ifndef __RESOURCEEDITORQT__LANDSCAPEEDITORSHORTCUTMANAGER__
#define __RESOURCEEDITORQT__LANDSCAPEEDITORSHORTCUTMANAGER__

#include "DAVAEngine.h"
#include "Base/Singleton.h"

#include <QShortcut>
#include <QPointer>

class LandscapeEditorShortcutManager : public DAVA::Singleton<LandscapeEditorShortcutManager>
{
public:
    LandscapeEditorShortcutManager(QWidget* shortcutsWidget);
    ~LandscapeEditorShortcutManager();

    QShortcut* GetShortcutByName(const DAVA::String& name);
    QShortcut* CreateOrUpdateShortcut(const DAVA::String& name, QKeySequence keySequence, bool autoRepeat = true, const DAVA::String& description = "");

    void SetHeightMapEditorShortcutsEnabled(bool enabled);

    void SetTileMaskEditorShortcutsEnabled(bool enabled);

    void SetBrushSizeShortcutsEnabled(bool enabled);

    void SetBrushImageSwitchingShortcutsEnabled(bool enabled);

    void SetTextureSwitchingShortcutsEnabled(bool enabled);

    void SetStrengthShortcutsEnabled(bool enabled);

    void SetAvgStrengthShortcutsEnabled(bool enabled);

private:
    DAVA::Map<DAVA::String, QShortcut*> shortcutsMap;

    void InitDefaultShortcuts();
    QPointer<QWidget> shortcutsWidget;
};

#endif /* defined(__RESOURCEEDITORQT__LANDSCAPEEDITORSHORTCUTMANAGER__) */
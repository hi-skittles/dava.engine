#include "LandscapeEditorShortcutManager.h"
#include "../Main/mainwindow.h"

LandscapeEditorShortcutManager::LandscapeEditorShortcutManager(QWidget* shortcutsWidget_)
    : shortcutsWidget(shortcutsWidget_)
{
    InitDefaultShortcuts();
}

LandscapeEditorShortcutManager::~LandscapeEditorShortcutManager()
{
}

QShortcut* LandscapeEditorShortcutManager::GetShortcutByName(const DAVA::String& name)
{
    DAVA::Map<DAVA::String, QShortcut*>::iterator it = shortcutsMap.find(name);
    if (it != shortcutsMap.end())
    {
        return it->second;
    }

    return NULL;
}

QShortcut* LandscapeEditorShortcutManager::CreateOrUpdateShortcut(const DAVA::String& name, QKeySequence keySequence,
                                                                  bool autoRepeat, const DAVA::String& description)
{
    QShortcut* shortcut = GetShortcutByName(name);
    if (shortcut == NULL)
    {
        shortcut = new QShortcut(QKeySequence(keySequence), shortcutsWidget.data(), 0, 0, Qt::ApplicationShortcut);
        shortcutsMap[name] = shortcut;
    }

    shortcut->setKey(QKeySequence(keySequence));
    shortcut->setEnabled(true);
    shortcut->setAutoRepeat(autoRepeat);
    shortcut->setWhatsThis(description.c_str());
    //shortcut disabled by default, will be changed during turning on/off of editors
    shortcut->setEnabled(false);

    return shortcut;
}

void LandscapeEditorShortcutManager::InitDefaultShortcuts()
{
    CreateOrUpdateShortcut(ResourceEditor::SHORTCUT_BRUSH_SIZE_INCREASE_SMALL, Qt::CTRL | Qt::Key_Equal);
    CreateOrUpdateShortcut(ResourceEditor::SHORTCUT_BRUSH_SIZE_DECREASE_SMALL, Qt::CTRL | Qt::Key_Minus);
    CreateOrUpdateShortcut(ResourceEditor::SHORTCUT_BRUSH_SIZE_INCREASE_LARGE, Qt::CTRL | Qt::ALT | Qt::Key_Equal);
    CreateOrUpdateShortcut(ResourceEditor::SHORTCUT_BRUSH_SIZE_DECREASE_LARGE, Qt::CTRL | Qt::ALT | Qt::Key_Minus);

    CreateOrUpdateShortcut(ResourceEditor::SHORTCUT_BRUSH_IMAGE_NEXT, Qt::CTRL | Qt::Key_BracketRight, false);
    CreateOrUpdateShortcut(ResourceEditor::SHORTCUT_BRUSH_IMAGE_PREV, Qt::CTRL | Qt::Key_BracketLeft, false);

    CreateOrUpdateShortcut(ResourceEditor::SHORTCUT_TEXTURE_NEXT, Qt::CTRL | Qt::ALT | Qt::Key_BracketRight, false);
    CreateOrUpdateShortcut(ResourceEditor::SHORTCUT_TEXTURE_PREV, Qt::CTRL | Qt::ALT | Qt::Key_BracketLeft, false);

    CreateOrUpdateShortcut(ResourceEditor::SHORTCUT_STRENGTH_INCREASE_SMALL, Qt::CTRL | Qt::Key_I);
    CreateOrUpdateShortcut(ResourceEditor::SHORTCUT_STRENGTH_DECREASE_SMALL, Qt::CTRL | Qt::Key_U);
    CreateOrUpdateShortcut(ResourceEditor::SHORTCUT_STRENGTH_INCREASE_LARGE, Qt::CTRL | Qt::ALT | Qt::Key_I);
    CreateOrUpdateShortcut(ResourceEditor::SHORTCUT_STRENGTH_DECREASE_LARGE, Qt::CTRL | Qt::ALT | Qt::Key_U);

    CreateOrUpdateShortcut(ResourceEditor::SHORTCUT_AVG_STRENGTH_INCREASE_SMALL, Qt::CTRL | Qt::Key_K);
    CreateOrUpdateShortcut(ResourceEditor::SHORTCUT_AVG_STRENGTH_DECREASE_SMALL, Qt::CTRL | Qt::Key_J);
    CreateOrUpdateShortcut(ResourceEditor::SHORTCUT_AVG_STRENGTH_INCREASE_LARGE, Qt::CTRL | Qt::ALT | Qt::Key_K);
    CreateOrUpdateShortcut(ResourceEditor::SHORTCUT_AVG_STRENGTH_DECREASE_LARGE, Qt::CTRL | Qt::ALT | Qt::Key_J);

    CreateOrUpdateShortcut(ResourceEditor::SHORTCUT_SET_ABSOLUTE, Qt::CTRL | Qt::Key_1, false);
    CreateOrUpdateShortcut(ResourceEditor::SHORTCUT_SET_RELATIVE, Qt::CTRL | Qt::Key_2, false);
    CreateOrUpdateShortcut(ResourceEditor::SHORTCUT_SET_AVERAGE, Qt::CTRL | Qt::Key_3, false);
    CreateOrUpdateShortcut(ResourceEditor::SHORTCUT_SET_ABS_DROP, Qt::CTRL | Qt::Key_4, false);
    CreateOrUpdateShortcut(ResourceEditor::SHORTCUT_SET_DROPPER, Qt::CTRL | Qt::Key_5, false);
    CreateOrUpdateShortcut(ResourceEditor::SHORTCUT_SET_COPY_PASTE, Qt::CTRL | Qt::Key_6, false);

    CreateOrUpdateShortcut(ResourceEditor::SHORTCUT_NORMAL_DRAW_TILEMASK, Qt::CTRL | Qt::Key_1, false);
    CreateOrUpdateShortcut(ResourceEditor::SHORTCUT_COPY_PASTE_TILEMASK, Qt::CTRL | Qt::Key_2, false);
}

void LandscapeEditorShortcutManager::SetHeightMapEditorShortcutsEnabled(bool enabled)
{
    shortcutsMap[ResourceEditor::SHORTCUT_SET_ABSOLUTE]->setEnabled(enabled);
    shortcutsMap[ResourceEditor::SHORTCUT_SET_RELATIVE]->setEnabled(enabled);
    shortcutsMap[ResourceEditor::SHORTCUT_SET_AVERAGE]->setEnabled(enabled);
    shortcutsMap[ResourceEditor::SHORTCUT_SET_ABS_DROP]->setEnabled(enabled);
    shortcutsMap[ResourceEditor::SHORTCUT_SET_DROPPER]->setEnabled(enabled);
    shortcutsMap[ResourceEditor::SHORTCUT_SET_COPY_PASTE]->setEnabled(enabled);
}

void LandscapeEditorShortcutManager::SetTileMaskEditorShortcutsEnabled(bool enabled)
{
    shortcutsMap[ResourceEditor::SHORTCUT_NORMAL_DRAW_TILEMASK]->setEnabled(enabled);
    shortcutsMap[ResourceEditor::SHORTCUT_COPY_PASTE_TILEMASK]->setEnabled(enabled);
}

void LandscapeEditorShortcutManager::SetBrushSizeShortcutsEnabled(bool enabled)
{
    shortcutsMap[ResourceEditor::SHORTCUT_BRUSH_SIZE_INCREASE_SMALL]->setEnabled(enabled);
    shortcutsMap[ResourceEditor::SHORTCUT_BRUSH_SIZE_DECREASE_SMALL]->setEnabled(enabled);
    shortcutsMap[ResourceEditor::SHORTCUT_BRUSH_SIZE_INCREASE_LARGE]->setEnabled(enabled);
    shortcutsMap[ResourceEditor::SHORTCUT_BRUSH_SIZE_DECREASE_LARGE]->setEnabled(enabled);
}

void LandscapeEditorShortcutManager::SetBrushImageSwitchingShortcutsEnabled(bool enabled)
{
    shortcutsMap[ResourceEditor::SHORTCUT_BRUSH_IMAGE_NEXT]->setEnabled(enabled);
    shortcutsMap[ResourceEditor::SHORTCUT_BRUSH_IMAGE_PREV]->setEnabled(enabled);
}

void LandscapeEditorShortcutManager::SetTextureSwitchingShortcutsEnabled(bool enabled)
{
    shortcutsMap[ResourceEditor::SHORTCUT_TEXTURE_NEXT]->setEnabled(enabled);
    shortcutsMap[ResourceEditor::SHORTCUT_TEXTURE_PREV]->setEnabled(enabled);
}

void LandscapeEditorShortcutManager::SetStrengthShortcutsEnabled(bool enabled)
{
    shortcutsMap[ResourceEditor::SHORTCUT_STRENGTH_INCREASE_SMALL]->setEnabled(enabled);
    shortcutsMap[ResourceEditor::SHORTCUT_STRENGTH_DECREASE_SMALL]->setEnabled(enabled);
    shortcutsMap[ResourceEditor::SHORTCUT_STRENGTH_INCREASE_LARGE]->setEnabled(enabled);
    shortcutsMap[ResourceEditor::SHORTCUT_STRENGTH_DECREASE_LARGE]->setEnabled(enabled);
}

void LandscapeEditorShortcutManager::SetAvgStrengthShortcutsEnabled(bool enabled)
{
    shortcutsMap[ResourceEditor::SHORTCUT_AVG_STRENGTH_INCREASE_SMALL]->setEnabled(enabled);
    shortcutsMap[ResourceEditor::SHORTCUT_AVG_STRENGTH_DECREASE_SMALL]->setEnabled(enabled);
    shortcutsMap[ResourceEditor::SHORTCUT_AVG_STRENGTH_INCREASE_LARGE]->setEnabled(enabled);
    shortcutsMap[ResourceEditor::SHORTCUT_AVG_STRENGTH_DECREASE_LARGE]->setEnabled(enabled);
}

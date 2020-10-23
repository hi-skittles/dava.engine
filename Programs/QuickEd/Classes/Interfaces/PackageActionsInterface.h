#pragma once

class QAction;

namespace Interfaces
{
class PackageActionsInterface
{
public:
    virtual QAction* GetImportPackageAction() = 0;
    virtual QAction* GetCutAction() = 0;
    virtual QAction* GetCopyAction() = 0;
    virtual QAction* GetPasteAction() = 0;
    virtual QAction* GetDuplicateAction() = 0;
    virtual QAction* GetDeleteAction() = 0;
    virtual QAction* GetJumpToPrototypeAction() = 0;
    virtual QAction* GetFindPrototypeInstancesAction() = 0;
};
}
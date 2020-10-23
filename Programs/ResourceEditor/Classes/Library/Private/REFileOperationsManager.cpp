#include "Classes/Library/Private/REFileOperationsManager.h"

#include <FileSystem/FilePath.h>

void REFileOperationsManager::RegisterFileOperation(std::shared_ptr<DAVA::REFileOperation> fileOperation)
{
    operations.push_back(fileOperation);
}

void REFileOperationsManager::UnregisterFileOperation(std::shared_ptr<DAVA::REFileOperation> fileOperation)
{
    operations.erase(std::remove(operations.begin(), operations.end(), fileOperation), operations.end());
}

DAVA::Vector<std::shared_ptr<DAVA::REFileOperation>> REFileOperationsManager::GetMatchedOperations(const DAVA::FilePath& contextFile)
{
    DAVA::Vector<std::shared_ptr<DAVA::REFileOperation>> result;

    QString fileExtension = QString::fromStdString(contextFile.GetExtension());
    for (const std::shared_ptr<DAVA::REFileOperation>& operation : operations)
    {
        QString targetFilters = operation->GetTargetFileFilter();
        if (targetFilters.indexOf(fileExtension) != -1)
        {
            result.push_back(operation);
        }
    }

    return result;
}

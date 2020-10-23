#pragma once

#include <REPlatform/Global/REFileOperationsIntarface.h>

#include <Base/BaseTypes.h>

class REFileOperationsManager : public DAVA::REFileOperationsInterface
{
public:
    void RegisterFileOperation(std::shared_ptr<DAVA::REFileOperation> fileOperation) override;
    void UnregisterFileOperation(std::shared_ptr<DAVA::REFileOperation> fileOperation) override;

    DAVA::Vector<std::shared_ptr<DAVA::REFileOperation>> GetMatchedOperations(const DAVA::FilePath& contextFile);

private:
    DAVA::Vector<std::shared_ptr<DAVA::REFileOperation>> operations;
};
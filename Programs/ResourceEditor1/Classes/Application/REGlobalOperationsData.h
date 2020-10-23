#pragma once

#include "TArc/DataProcessing/DataNode.h"
#include <memory>

class GlobalOperations;
class REGlobalOperationsData : public DAVA::TArc::DataNode
{
public:
    void SetGlobalOperations(const std::shared_ptr<GlobalOperations>& globalOperations);
    const std::shared_ptr<GlobalOperations>& GetGlobalOperations() const;

private:
    std::shared_ptr<GlobalOperations> globalOperations = nullptr;

    DAVA_VIRTUAL_REFLECTION_IN_PLACE(REGlobalOperationsData, DAVA::TArc::DataNode)
    {
        DAVA::ReflectionRegistrator<REGlobalOperationsData>::Begin()
        .End();
    }
};

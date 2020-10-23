#include "Classes/Application/REGlobalOperationsData.h"
#include "Classes/Qt/GlobalOperations.h"

void REGlobalOperationsData::SetGlobalOperations(const std::shared_ptr<GlobalOperations>& globalOperations_)
{
    globalOperations = globalOperations_;
}

const std::shared_ptr<GlobalOperations>& REGlobalOperationsData::GetGlobalOperations() const
{
    return globalOperations;
}

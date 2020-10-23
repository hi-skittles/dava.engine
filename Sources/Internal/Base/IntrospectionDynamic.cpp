#include "Base/IntrospectionDynamic.h"
#include "Base/Introspection.h"

namespace DAVA
{
InspMemberDynamic::InspMemberDynamic(const char* _name, const InspDesc& _desc, const MetaInfo* _type, int _flags, InspInfoDynamic* _dynamicInfo)
    : InspMember(_name, _desc, 0, _type, _flags)
    , dynamicInfo(_dynamicInfo)
{
    if (NULL != dynamicInfo)
    {
        dynamicInfo->memberDynamic = this;
    }
}

InspMemberDynamic::~InspMemberDynamic()
{
    if (NULL != dynamicInfo)
    {
        delete dynamicInfo;
    }
}

const InspMemberDynamic* InspMemberDynamic::Dynamic() const
{
    return this;
}

InspInfoDynamic* InspMemberDynamic::GetDynamicInfo() const
{
    return dynamicInfo;
}
};

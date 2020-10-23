#ifndef __DAVAENGINE_INTROSPECTION_DYNAMIC_H__
#define __DAVAENGINE_INTROSPECTION_DYNAMIC_H__

#include "Base/BaseTypes.h"
#include "Base/IntrospectionBase.h"

namespace DAVA
{
class InspInfoDynamic;

class InspMemberDynamic : public InspMember
{
public:
    InspMemberDynamic(const char* _name, const InspDesc& _desc, const MetaInfo* _type, int _flags, InspInfoDynamic* _dynamicInfo);
    ~InspMemberDynamic();

    virtual const InspMemberDynamic* Dynamic() const override;
    InspInfoDynamic* GetDynamicInfo() const;

    void* Pointer(void* object) const override;
    void* Data(void* object) const override;

protected:
    InspInfoDynamic* dynamicInfo;
};

inline void* InspMemberDynamic::Pointer(void* object) const
{
    return nullptr;
}

inline void* InspMemberDynamic::Data(void* object) const
{
    return nullptr;
}
} // namespace DAVA

#endif // __DAVAENGINE_INTROSPECTION_DYNAMIC_H__

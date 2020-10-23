#include "DAVAEngine.h"
#include "QtPropertyDataIntrospection.h"
#include "QtPropertyDataDavaKeyedArchive.h"
#include "QtPropertyDataInspMember.h"
#include "QtPropertyDataInspDynamic.h"
#include "QtPropertyDataInspColl.h"

QtPropertyDataIntrospection::QtPropertyDataIntrospection(const DAVA::FastName& name, void* _object, const DAVA::InspInfo* _info, bool autoAddChilds)
    : QtPropertyData(name)
    , object(_object)
    , info(_info)
{
    if (autoAddChilds)
    {
        while (NULL != _info && NULL != object)
        {
            for (DAVA::int32 i = 0; i < _info->MembersCount(); ++i)
            {
                const DAVA::InspMember* member = _info->Member(i);
                if (NULL != member)
                {
                    AddMember(member);
                }
            }

            _info = _info->BaseInfo();
        }
    }

    SetEnabled(false);
}

QtPropertyDataIntrospection::~QtPropertyDataIntrospection()
{
}

const DAVA::MetaInfo* QtPropertyDataIntrospection::MetaInfo() const
{
    if (NULL != info)
    {
        return info->Type();
    }

    return NULL;
}

QtPropertyData* QtPropertyDataIntrospection::CreateMemberData(const DAVA::FastName& name, void* _object, const DAVA::InspMember* member)
{
    void* memberObject = member->Data(_object);
    int memberFlags = member->Flags();
    const DAVA::MetaInfo* memberMetaInfo = member->Type();
    const DAVA::InspInfo* memberIntrospection = memberMetaInfo->GetIntrospection(memberObject);
    bool isKeyedArchive = false;

    QtPropertyData* retData = NULL;
    // keyed archive
    if (NULL != memberIntrospection && (memberIntrospection->Type() == DAVA::MetaInfo::Instance<DAVA::KeyedArchive>()))
    {
        retData = new QtPropertyDataDavaKeyedArcive(name, (DAVA::KeyedArchive*)memberObject);
    }
    // introspection
    else if (NULL != memberObject && NULL != memberIntrospection)
    {
        retData = new QtPropertyDataIntrospection(name, memberObject, memberIntrospection);
    }
    // any other value
    else
    {
        // pointer
        if (memberMetaInfo->IsPointer())
        {
            QString s;
            retData = new QtPropertyData(name, s.sprintf("[%p] Pointer", memberObject));
            retData->SetEnabled(false);
        }
        // other value
        else
        {
            // collection
            if (member->Collection() && !isKeyedArchive)
            {
                retData = new QtPropertyDataInspColl(name, memberObject, member->Collection());
            }
            // dynamic
            else if (NULL != member->Dynamic())
            {
                retData = new QtPropertyData(DAVA::FastName("Dynamic data"), "Dynamic data");
                retData->SetEnabled(false);

                DAVA::InspInfoDynamic* dynamicInfo = member->Dynamic()->GetDynamicInfo();
                if (NULL != dynamicInfo)
                {
                    DAVA::InspInfoDynamic::DynamicData ddata = dynamicInfo->Prepare(_object);
                    DAVA::Vector<DAVA::FastName> membersList = dynamicInfo->MembersList(ddata); // this function can be slow
                    for (size_t i = 0; i < membersList.size(); ++i)
                    {
                        int memberFlags = dynamicInfo->MemberFlags(ddata, membersList[i]);
                        if (memberFlags & DAVA::I_VIEW)
                        {
                            QtPropertyDataInspDynamic* dynamicMember = new QtPropertyDataInspDynamic(membersList[i], dynamicInfo, ddata);
                            if (!(memberFlags & DAVA::I_EDIT))
                            {
                                dynamicMember->SetEnabled(false);
                            }

                            retData->ChildAdd(std::unique_ptr<QtPropertyData>(dynamicMember));
                        }
                    }
                }
            }
            // variant
            else
            {
                QtPropertyDataInspMember* childData = new QtPropertyDataInspMember(name, _object, member);
                if (memberFlags & DAVA::I_EDIT)
                {
                    // check if description has some predefines enum values
                    const DAVA::InspDesc& desc = member->Desc();
                    childData->SetInspDescription(desc);
                }

                retData = childData;
            }
        }
    }

    if (NULL != retData)
    {
        if (!(memberFlags & DAVA::I_EDIT))
            retData->SetEditable(false);
    }

    return retData;
}

void QtPropertyDataIntrospection::AddMember(const DAVA::InspMember* member)
{
    if ((member->Flags() & DAVA::I_VIEW))
    {
        QtPropertyData* data = CreateMemberData(member->Name(), object, member);
        void* memberObject = member->Data(object);
        const DAVA::MetaInfo* memberMetaInfo = member->Type();
        const DAVA::InspInfo* memberIntrospection = memberMetaInfo->GetIntrospection(memberObject);

        ChildAdd(std::unique_ptr<QtPropertyData>(data));
        //condition for variant
        if ((!memberMetaInfo->IsPointer()) && (!member->Collection()) &&
            (NULL == memberIntrospection || (memberIntrospection->Type() != DAVA::MetaInfo::Instance<DAVA::KeyedArchive>())))
        {
            QtPropertyDataInspMember* childData = dynamic_cast<QtPropertyDataInspMember*>(data);
            if (NULL != childData)
            {
                childVariantMembers.insert(childData, member);
            }
        }
    }
}

QVariant QtPropertyDataIntrospection::GetValueInternal() const
{
    return QVariant(info->Name().c_str());
}

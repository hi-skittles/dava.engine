#ifndef __DAVAFRAMEWORK_OBJECTHANDLE_H__
#define __DAVAFRAMEWORK_OBJECTHANDLE_H__

namespace DAVA
{
struct MetaInfo;
class InspBase;
class InspInfo;
class ObjectHandle final
{
public:
    ObjectHandle() = default;
    ObjectHandle(void* object, const DAVA::MetaInfo* objectType);
    ObjectHandle(DAVA::InspBase* object);

    bool IsValid() const;
    void* GetObjectPointer() const;
    const MetaInfo* GetObjectType() const;
    const InspInfo* GetIntrospection() const;

private:
    void* object = nullptr;
    const DAVA::MetaInfo* objectType = nullptr;
};

inline void* ObjectHandle::GetObjectPointer() const
{
    return object;
}

inline const DAVA::MetaInfo* ObjectHandle::GetObjectType() const
{
    return objectType;
}
}

#endif // __DAVAFRAMEWORK_OBJECTHANDLE_H__
#ifndef __DAVAENGINE_DATA_H__
#define __DAVAENGINE_DATA_H__

#include "Base/BaseTypes.h"
#include "Base/BaseObject.h"

namespace DAVA
{
/** 
	\ingroup baseobjects
	\brief class to store static or dynamic byte array
*/

class Data : public BaseObject
{
protected:
    ~Data();

public:
    Data(uint32 _size);
    Data(uint8* _data, uint32 _size);

    /**
        \brief Get pointer to data array
        \returns pointer
     */
    inline const uint8* GetPtr() const;
    inline uint8* GetPtr();
    /**
        \brief Get size of this date object
        \returns size
     */
    inline uint32 GetSize() const;

private:
    uint8* data;
    uint32 size;
};

// Implementation
inline uint8* Data::GetPtr()
{
    return data;
}

inline const uint8* Data::GetPtr() const
{
    return data;
}

inline uint32 Data::GetSize() const
{
    return size;
}
}; 


#endif // __DAVAENGINE_DATA_H__

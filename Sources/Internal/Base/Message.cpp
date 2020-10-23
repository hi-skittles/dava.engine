#include "Base/Message.h"
#include "Base/BaseObject.h"

namespace DAVA
{
MessageBase::~MessageBase()
{
}
//void Message::SetSelector(DAVA::BaseObject *_pObj, void (BaseObject::*_pFunc)(BaseObject*, void*, void*), void * _pUserData)
//{
//	pObj = _pObj;
//	pFunc = _pFunc;
//	pUserData = _pUserData;
//}

//void Message::Call(BaseObject * pCaller)
//{
//	if(pObj && pFunc)
//		(pObj->*pFunc)(pCaller, pUserData, NULL);
//	else if (pFunc2)
//		pFunc2(pCaller, pUserData, NULL);
//}
//
//void Message::Call(BaseObject * pCaller, void * callerData)
//{
//	if(pObj && pFunc)
//		(pObj->*pFunc)(pCaller, pUserData, callerData);
//	else if (pFunc2)
//		pFunc2(pCaller, pUserData, callerData);
//}
};
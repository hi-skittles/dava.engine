#ifndef __DAVAENGINE_GAME_OBJECT_MANAGER_H__
#define __DAVAENGINE_GAME_OBJECT_MANAGER_H__

#include "Base/BaseTypes.h"
#include "Base/BaseObject.h"
#include "Animation/Animation.h"
#include "Animation/AnimatedObject.h"
#include "Scene2D/GameObject.h"
#include <stack>
#include <typeinfo>

namespace DAVA
{
/**
	\ingroup scene2d
	\brief base container in 2d scene hierarchy
	Class is intended for base management of gameobjects. 
	It manages object priorities, and handle all major GameObject events. 
	You can also use this class to enumerate game objects by types and tags. 
 */
class GameObjectManager : public BaseObject
{
protected:
    GameObjectManager();
    virtual ~GameObjectManager();

    SpriteDrawState drawState;

    Vector2 cameraPosition;
    Vector2 cameraScale;

public:
    static GameObjectManager* Create();

    virtual void AddObject(GameObject* _object);
    virtual void RemoveObject(GameObject* _object);
    virtual void RemoveAllObjects();
    inline int32 GetObjectsCount();

    virtual void Update(float32 timeElapsed);
    virtual void Draw();

    virtual void SetCameraPosition(float32 cameraPositionX, float32 cameraPositionY);
    virtual void SetCameraScale(float32 cameraScale);
    virtual float32 GetCameraScale() const;

    /*	
		Functions to enumerate objects
	 */
    /*
	 \brief	Function to get objects to container. I works for all containers that have push_back and clear functions.
			When you use this function be sure that you always get new list of objects before manipulating with them
			Because if anyone delete the object manager, your container can contain bad ptrs
	 */
    template <template <typename> class Container, class T>
    void GetObjectsToContainer(Container<T>& container);

    /*
	 \brief	You should call EnumerateObjects first if you want to use function EnumerationGetNext
	 */
    inline void EnumerateObjects();
    /*
	 \brief	Function return next object in list. When using this function be sure that u enumerated all objects
			If you called enumerate objects once you should enumerate all objects in list
			Do not try to get object on next update because after update list of objects can be changed
	 \return next actual game object in list, 0 if we reached the end of the list 
	 */
    inline GameObject* EnumerationGetNext();

    /*
	 \brief Get next enumeration object with desired group id
	 \return next actual game object, 0 if we reached the end of the list 
	 */
    inline GameObject* EnumerationGetNext(int32 groupId);

    void RecalcObjectsHierarchy();
    void RecalcObjectHierarchy(GameObject* object) const;

protected:
    void RealAddObject(GameObject* _object);

    void ChangeObjectPriority(GameObject* _object);
    void MarkToChangeObjectPriority(GameObject* _object);
    void SortObjects();

    List<GameObject*> objects;
    List<GameObject*>::iterator objectEnumerator;
    bool isInUpdate;
    std::stack<GameObject*> changesStack;
    void ProcessChangesStack();

    friend class GameObject;
};

template <template <typename> class Container, class T>
void GameObjectManager::GetObjectsToContainer(Container<T>& container)
{
    container.clear();
    List<GameObject*>::const_iterator end = objects.end();
    for (List<GameObject*>::iterator t = objects.begin(); t != end; ++t)
    {
        GameObject* obj = *t;
        if (obj->dead)
            continue;

        T res = dynamic_cast<T>(obj);
        if (res)
            container.push_back(res);
    }
}

inline void GameObjectManager::EnumerateObjects()
{
    objectEnumerator = objects.begin();
}

inline GameObject* GameObjectManager::EnumerationGetNext()
{
    List<GameObject*>::const_iterator end = objects.end();

    while (objectEnumerator != end)
    {
        GameObject* obj = *objectEnumerator;
        ++objectEnumerator;
        if (obj->dead)
            continue;
        return obj;
    }
    return 0;
}

inline GameObject* GameObjectManager::EnumerationGetNext(int32 groupId)
{
    List<GameObject*>::const_iterator end = objects.end();

    while (objectEnumerator != end)
    {
        GameObject* obj = *objectEnumerator;
        ++objectEnumerator;
        if (obj->dead)
            continue;
        if (obj->GetGroupId() == groupId)
            return obj;
    }
    return 0;
}

inline int32 GameObjectManager::GetObjectsCount()
{
    return static_cast<int32>(objects.size());
}
};
#endif // __DAVAENGINE_GAME_OBJECT_MANAGER_H__
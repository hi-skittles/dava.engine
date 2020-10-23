#include "Scene2D/GameObjectManager.h"
#include "Logger/Logger.h"
#include "Collision/CollisionObject2.h"
#include "Render/2D/Systems/RenderSystem2D.h"
#include "Render/Renderer.h"

/*
	/TODO optimize object manager
	Optimize sort
	Optimize remove
*/

namespace DAVA
{
GameObjectManager::GameObjectManager() //-V730 do not init objectEnumerator
: cameraScale(1.f, 1.f)
{
    isInUpdate = false;
}

GameObjectManager::~GameObjectManager()
{
    RemoveAllObjects();
}

void GameObjectManager::RemoveAllObjects()
{
    for (List<GameObject*>::iterator currentPos = objects.begin(); currentPos != objects.end(); ++currentPos)
    {
        GameObject* o = *currentPos;
        o->SetManager(0);
        SafeRelease(o);
    }
    objects.clear();
}

GameObjectManager* GameObjectManager::Create()
{
    GameObjectManager* manager = new GameObjectManager();
    return manager;
}

void GameObjectManager::MarkToChangeObjectPriority(GameObject* _object)
{
    if (isInUpdate)
    {
        changesStack.push(_object);
        _object->priorityChanged = true;
    }
    else
    {
        ChangeObjectPriority(_object);
    }
}

void GameObjectManager::ChangeObjectPriority(GameObject* _object)
{
    for (List<GameObject*>::iterator t = objects.begin(); t != objects.end(); ++t)
    {
        if (*t == _object)
        {
            objects.erase(t);
            break;
        }
    }

    List<GameObject*>::iterator currentPos = objects.begin();
    int pos = 0;
    for (; currentPos != objects.end(); ++currentPos)
    {
        GameObject* curObject = *currentPos;

        if (curObject->GetPriority() > _object->GetPriority())
        {
            break;
        }
        pos++;
    }
    objects.insert(currentPos, _object);
}

void GameObjectManager::AddObject(GameObject* _object)
{
    /*if (isInUpdate)
	{
		changesStack.push(_object);
		_object->addedObject = true;
	}else
	{
		//if (object
		//_object->GetParent()->RecalcHierarchy(<#Sprite parentDrawState#>)*/
    RealAddObject(_object);

    List<GameObject*>& childs = _object->GetChildren();

    for (List<GameObject*>::iterator t = childs.begin(); t != childs.end(); ++t)
    {
        GameObject* child = *t;
        this->AddObject(child);
    }
    //}
}

void GameObjectManager::RealAddObject(GameObject* _object)
{
    if (!_object)
        return;

    if (_object->dead) // if object is dead we mark that it should be added on deletion from prev. manager
    {
        _object->nextManager = this;
        return;
    }
    else
    {
        DVASSERT(_object->GetManager() == 0);
    }

    _object->Retain();

    //int objectsCount = objects.size();
    List<GameObject*>::iterator currentPos = objects.begin();

    //Logger::FrameworkDebug("myp: %d\n", _object->GetPriority());
    int pos = 0;
    for (; currentPos != objects.end(); ++currentPos)
    {
        GameObject* curObject = *currentPos;
        if (curObject->GetPriority() > _object->GetPriority())
        {
            //Logger::FrameworkDebug("op: %d\n", curObject->GetPriority());

            break;
        }
        pos++;
    }
    //Logger::FrameworkDebug("added to pos: %d\n", pos);
    objects.insert(currentPos, _object);
    _object->SetManager(this);
    RecalcObjectHierarchy(_object);
}

void GameObjectManager::RemoveObject(GameObject* _object)
{
    if (_object && _object->GetManager() == this && (!_object->dead))
    {
        _object->dead = true;

        changesStack.push(_object);
    }
}

bool PrioritySortFn(GameObject* o1, GameObject* o2)
{
    return o1->GetPriority() < o2->GetPriority();
}

/*
void GameObjectManager::SortObjects()
{
	std::sort(objects.begin(), objects.end(), PrioritySortFn);
}
*/

void GameObjectManager::RecalcObjectHierarchy(GameObject* object) const
{
    const GameObject* objParent = object->GetParent();
    object->RecalcHierarchy(objParent ? objParent->globalDrawState : drawState);
}

void GameObjectManager::RecalcObjectsHierarchy()
{
    List<GameObject*>::iterator currentPos_end = objects.end();
    for (List<GameObject*>::iterator currentPos = objects.begin(); currentPos != currentPos_end; ++currentPos)
    {
        GameObject* object = *currentPos;
        if (object->dead)
            continue;
        if (object->GetParent() == 0)
            object->RecalcHierarchy(drawState);
    }
}

void GameObjectManager::Update(float32 timeElapsed)
{
    isInUpdate = true;
    // Than perform usual update
    for (List<GameObject*>::iterator currentPos = objects.begin(); currentPos != objects.end(); ++currentPos)
    {
        GameObject* object = *currentPos;
        if (object->dead)
            continue;
        if (object->GetParent() == 0)
            object->Update(timeElapsed);
    }

    RecalcObjectsHierarchy();

    for (List<GameObject*>::iterator currentPos = objects.begin(); currentPos != objects.end(); ++currentPos)
    {
        GameObject* object = *currentPos;
        if (object->dead)
            continue;
        object->CollisionPhase();
    }

    isInUpdate = false;

    ProcessChangesStack();
}

void GameObjectManager::ProcessChangesStack()
{
    while (changesStack.size() > 0)
    {
        GameObject* object = changesStack.top();
        changesStack.pop();
        if (object->priorityChanged)
        {
            ChangeObjectPriority(object);
            object->priorityChanged = false;
        }
        else if (object->dead)
        {
            // delete objects
            for (List<GameObject*>::iterator t = objects.begin(); t != objects.end(); ++t)
            {
                if (*t == object)
                {
                    objects.erase(t);
                    break;
                }
            }
            object->dead = false;
            object->SetManager(0);

            if (object->nextManager)
            {
                GameObjectManager* nextManager = object->nextManager;
                object->nextManager = 0;
                object->SetManager(nextManager);
            }

            SafeRelease(object);
        }
    }
}

void GameObjectManager::Draw()
{
    Matrix4 worldMx;
    worldMx.BuildTranslation(Vector3(cameraPosition.x, cameraPosition.y, 0.f));
    worldMx = worldMx * Matrix4::MakeScale(Vector3(cameraScale.x, cameraScale.y, 1.f));
    Renderer::GetDynamicBindings().SetDynamicParam(DynamicBindings::PARAM_WORLD, &worldMx, DynamicBindings::UPDATE_SEMANTIC_ALWAYS);

    const List<GameObject*>::iterator currentObjEnd = objects.end();
    for (List<GameObject*>::iterator currentObj = objects.begin(); currentObj != currentObjEnd; ++currentObj)
    {
        GameObject* object = *currentObj;
        if (object->dead)
            continue;
        else
            object->Draw();
    }
}

void GameObjectManager::SetCameraPosition(float32 _cameraPositionX, float32 _cameraPositionY)
{
    // 	drawState.position.x = -_cameraPositionX;
    // 	drawState.position.y = -_cameraPositionY;
    cameraPosition.x = -_cameraPositionX;
    cameraPosition.y = -_cameraPositionY;
}

void GameObjectManager::SetCameraScale(float32 _cameraScale)
{
    //	drawState.scale.x = drawState.scale.y = _cameraScale;
    cameraScale.x = cameraScale.y = _cameraScale;
}

float32 GameObjectManager::GetCameraScale() const
{
    return cameraScale.x; // > 0 ? drawState.scale.x : 1.f;
}
};
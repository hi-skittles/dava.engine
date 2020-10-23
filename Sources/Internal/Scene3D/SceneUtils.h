#pragma once

#include "Base/BaseTypes.h"

namespace DAVA
{
class RenderObject;
class RenderBatch;
class Entity;
class Scene;

class SceneUtils final
{
public:
    /*
     \brief Iterates over all scene nodes, searches nodes with speciefic names and makes new nodes with lods.
     \return true if combination was successfull
    */
    static bool CombineLods(Scene* scene);

    /*
     \brief Creates Entity name for speciefic lod index by pattern.
     */
    static String LodNameForIndex(const String& pattern, uint32 lodIndex);

    /*
     \brief Finds all enitities with SameName_lod0 and then iterates over that nodes and searches similar ones with another lods.
            Then takes all found nodes with the same name and different lods and makes one new entity with multiple render barches for
            different lods. It is our convience - "EntityName_lod%d". If we'll change it - we should write new function like this.
     \return true if combination was successfull
     */
    static bool CombineEntityLods(Entity* forRootNode);

    /*
     \brief Iterates ecursive over all nodes from currentNode and deeper.
            Accumulates transform from parent to currentNode.
            Bakes accumulated transform into currentNode.
     
            In result we have all entities from currentNode and deeper with IDENTITY transforms and transformed geometry which looks the same as before.
    */
    static void BakeTransformsUpToFarParent(Entity* parent, Entity* currentNode);

    /*
     brief Takes all node childs animations and puts them into node.
     */
    static void CollapseAnimationsUpToFarParent(Entity* node, Entity* parent);

    /*
     \brief Remove entitis without children and without components from scene
     */
    static bool RemoveEmptyEntities(Scene* scene);
};

} //namespace DAVA

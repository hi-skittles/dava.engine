#ifndef __DAVAENGINE_QUADTREE_H__
#define __DAVAENGINE_QUADTREE_H__

#include "Base/BaseTypes.h"
#include "Base/BaseObject.h"

namespace DAVA
{
template <typename T>
struct AbstractQuadTreeNode
{
    T data;
    AbstractQuadTreeNode<T>** children; //*[4]
    AbstractQuadTreeNode<T>* parent;

    inline AbstractQuadTreeNode();
    inline ~AbstractQuadTreeNode();

    inline bool IsTerminalLeaf() const;
};

template <typename T>
class AbstractQuadTree
{
public:
    inline AbstractQuadTree();
    inline ~AbstractQuadTree();

    inline void Init(uint32 depth);
    inline void Clear();
    inline AbstractQuadTreeNode<T>* GetRoot() const;
    inline uint32 GetLevelCount() const;

private:
    void BuildTreeNode(AbstractQuadTreeNode<T>* node, int32 currentDepth);

private:
    AbstractQuadTreeNode<T>* root;
    uint32 treeDepth;
};

template <typename T>
inline AbstractQuadTreeNode<T>::AbstractQuadTreeNode()
{
    children = NULL;
    parent = NULL;
}

template <typename T>
inline AbstractQuadTreeNode<T>::~AbstractQuadTreeNode()
{
    if (!IsTerminalLeaf())
    {
        for (uint32 i = 0; i < 4; ++i)
        {
            SafeDelete(children[i]);
        }
    }

    SafeDeleteArray(children);
}

template <typename T>
inline bool AbstractQuadTreeNode<T>::IsTerminalLeaf() const
{
    return (NULL == children);
}

template <typename T>
inline AbstractQuadTree<T>::AbstractQuadTree()
    :
    root(NULL)
    ,
    treeDepth(0)
{
}

template <typename T>
inline AbstractQuadTree<T>::~AbstractQuadTree()
{
    Clear();
}

template <typename T>
inline void AbstractQuadTree<T>::Init(uint32 depth)
{
    Clear();

    treeDepth = depth;

    if (treeDepth > 0)
    {
        root = new AbstractQuadTreeNode<T>();
        BuildTreeNode(root, treeDepth);
    }
}

template <typename T>
inline void AbstractQuadTree<T>::Clear()
{
    SafeDelete(root);
    treeDepth = 0;
}

template <typename T>
inline AbstractQuadTreeNode<T>* AbstractQuadTree<T>::GetRoot() const
{
    return root;
}

template <typename T>
inline uint32 AbstractQuadTree<T>::GetLevelCount() const
{
    return treeDepth;
}

template <typename T>
void AbstractQuadTree<T>::BuildTreeNode(AbstractQuadTreeNode<T>* node, int32 currentDepth)
{
    currentDepth--;

    if (currentDepth >= 0)
    {
        node->children = new AbstractQuadTreeNode<T>*[4];
        for (uint32 i = 0; i < 4; ++i)
        {
            node->children[i] = new AbstractQuadTreeNode<T>();
            node->children[i]->parent = node;

            BuildTreeNode(node->children[i], currentDepth);
        }
    }
}
};

#endif

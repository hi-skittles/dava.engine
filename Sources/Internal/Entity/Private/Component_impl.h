#pragma once

namespace DAVA
{
inline Entity* Component::GetEntity() const
{
    return entity;
};

template <template <typename> class Container, class T>
void Component::GetDataNodes(Container<T>& container)
{
    Set<DataNode*> objects;
    GetDataNodes(objects);

    Set<DataNode*>::const_iterator end = objects.end();
    for (Set<DataNode*>::iterator t = objects.begin(); t != end; ++t)
    {
        DataNode* obj = *t;

        T res = dynamic_cast<T>(obj);
        if (res != nullptr)
        {
            container.push_back(res);
        }
    }
}

} // namespace DAVA
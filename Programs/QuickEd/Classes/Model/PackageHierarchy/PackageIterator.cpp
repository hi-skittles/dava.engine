#include "PackageIterator.h"
#include "PackageNode.h"
#include "PackageBaseNode.h"

const PackageIterator::MatchFunction PackageIterator::defaultFunction = [](const PackageBaseNode*) -> bool { return true; };

struct PackageIterator::IteratorData
{
    using ParentIndexes = DAVA::Stack<DAVA::uint32>;
    bool Accaptable() const;
    PackageBaseNode* Next();
    PackageBaseNode* Previous();
    void InitFromNode(PackageBaseNode* node);

    MatchFunction func = defaultFunction;
    PackageBaseNode* currentNode = nullptr;

private:
    DAVA::uint32 currentIndex = 0;
    ParentIndexes parentIndexes;
};

PackageIterator::PackageIterator(const PackageIterator& it)
    : impl(new IteratorData(*it.impl))
{
    DVASSERT(nullptr != impl->func);
}

PackageIterator::PackageIterator(PackageBaseNode* node, MatchFunction func_)
    : impl(new IteratorData())
{
    DVASSERT(nullptr != node);
    DVASSERT(nullptr != func_);
    impl->func = func_;
    impl->InitFromNode(node);
}

PackageIterator::~PackageIterator() = default;

bool PackageIterator::IsValid() const
{
    return impl->currentNode != nullptr;
}

void PackageIterator::SetMatchFunction(MatchFunction func)
{
    impl->func = func;
}

PackageIterator& PackageIterator::operator=(const PackageIterator& it)
{
    *impl.get() = *it.impl.get();
    return *this;
}

/*!
 The prefix ++ operator (++it) advances the iterator to the next matching item
 and returns a reference to the resulting iterator.
 Sets the current pointer to 0 if the current item is the last matching item.
 */

PackageIterator& PackageIterator::operator++()
{
    if (IsValid())
    {
        do
        {
            impl->currentNode = impl->Next();
        } while (IsValid() && !impl->Accaptable());
    }
    return *this;
}

PackageIterator& PackageIterator::operator+=(int n)
{
    if (n < 0)
    {
        return (*this) -= (-n);
    }
    while (IsValid() && n--)
    {
        ++(*this);
    }
    return *this;
}

/*!
 The prefix -- operator (--it) advances the iterator to the previous matching item
 and returns a reference to the resulting iterator.
 Sets the current pointer to 0 if the current item is the first matching item.
 */

PackageIterator& PackageIterator::operator--()
{
    if (IsValid())
    {
        do
        {
            impl->currentNode = impl->Previous();
        } while (IsValid() && !impl->Accaptable());
    }
    return *this;
}

PackageIterator& PackageIterator::operator-=(int n)
{
    if (n < 0)
    {
        return (*this) += (-n);
    }
    while (IsValid() && n--)
    {
        --(*this);
    }
    return *this;
}

PackageBaseNode* PackageIterator::operator*() const
{
    return impl->currentNode;
}

bool PackageIterator::IteratorData::Accaptable() const
{
    return func(currentNode);
}

void PackageIterator::IteratorData::InitFromNode(PackageBaseNode* node)
{
    currentNode = node;
    PackageBaseNode* parent = node->GetParent();

    ParentIndexes parentIndexesReverse;

    while (nullptr != parent)
    {
        parentIndexesReverse.push(parent->GetIndex(node));
        node = parent;
        parent = parent->GetParent();
    }
    while (!parentIndexesReverse.empty())
    {
        parentIndexes.push(parentIndexesReverse.top());
        parentIndexesReverse.pop();
    }
    currentIndex = parentIndexes.top();
    parentIndexes.pop();
}

PackageBaseNode* PackageIterator::IteratorData::Next()
{
    DVASSERT(nullptr != currentNode && "calling Next for invalid iterator");
    PackageBaseNode* next = nullptr;
    if (currentNode->GetCount())
    {
        // walk the child
        parentIndexes.push(currentIndex);
        currentIndex = 0;
        next = currentNode->Get(0);
    }
    else
    {
        // walk the sibling
        PackageBaseNode* parent = currentNode->GetParent();
        DVASSERT(nullptr != parent); //orphan without parent and childs
        DAVA::uint32 count = parent->GetCount();
        next = currentIndex < (count - 1) ? parent->Get(currentIndex + 1) : nullptr;
        while (next == nullptr)
        {
            // if we had no sibling walk up the parent and try the sibling of that
            parent = parent->GetParent();
            if (nullptr == parent)
            {
                DVASSERT(parentIndexes.empty());
                return nullptr;
            }
            DVASSERT(!parentIndexes.empty());
            currentIndex = parentIndexes.top();
            parentIndexes.pop();
            if (nullptr != parent)
            {
                count = parent->GetCount();
                next = currentIndex < (count - 1) ? parent->Get(currentIndex + 1) : nullptr;
            }
        }
        DVASSERT(nullptr != next);
        ++currentIndex;
    }
    return next;
}

PackageBaseNode* PackageIterator::IteratorData::Previous()
{
    DVASSERT(nullptr != currentNode && "calling Previous for invalid iterator");

    PackageBaseNode* prev = nullptr;
    PackageBaseNode* parent = currentNode->GetParent();
    if (parent == nullptr)
    {
        DVASSERT(currentIndex == 0 && parentIndexes.empty());
        return nullptr;
    }
    // walk the previous sibling
    prev = currentIndex > 0 ? parent->Get(currentIndex - 1) : nullptr;
    if (nullptr != prev)
    {
        // Yes, we had a previous sibling but we need go down to the last leafnode.
        --currentIndex;
        while (nullptr != prev && prev->GetCount())
        {
            parentIndexes.push(currentIndex);
            currentIndex = prev->GetCount() - 1;
            prev = prev->Get(currentIndex);
        }
    }
    else if (nullptr != parent->GetParent())
    {
        DVASSERT(!parentIndexes.empty());
        currentIndex = parentIndexes.top();
        parentIndexes.pop();
        prev = parent;
    }
    else
    {
        DVASSERT(parentIndexes.empty());
        return nullptr;
    }
    return prev;
}

PackageIterator operator+(PackageIterator iter, int n)
{
    while (iter.IsValid() && n++)
    {
        ++iter;
    }
    return iter;
}

PackageIterator operator-(PackageIterator iter, int n)
{
    while (iter.IsValid() && n--)
    {
        --iter;
    }
    return iter;
}

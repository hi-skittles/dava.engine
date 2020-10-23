#ifndef __QUICKED_PACKAGE_ITERATOR_H__
#define __QUICKED_PACKAGE_ITERATOR_H__

#include "Base/BaseTypes.h"
#include "Functional/Function.h"

class PackageNode;
class PackageBaseNode;

class PackageIterator
{
public:
    using MatchFunction = DAVA::Function<bool(const PackageBaseNode*)>;
    PackageIterator(const PackageIterator& it);
    explicit PackageIterator(PackageBaseNode* node, MatchFunction func = defaultFunction);
    ~PackageIterator();
    bool IsValid() const;
    void SetMatchFunction(MatchFunction func);

    PackageIterator& operator=(const PackageIterator& it);

    PackageIterator& operator++();
    const PackageIterator operator++(int);
    PackageIterator& operator+=(int n);

    PackageIterator& operator--();
    const PackageIterator operator--(int);
    PackageIterator& operator-=(int n);

    PackageBaseNode* operator*() const;

private:
    struct IteratorData;
    std::unique_ptr<IteratorData> impl;
    static const MatchFunction defaultFunction;
};

inline const PackageIterator PackageIterator::operator++(int)
{
    PackageIterator it = *this;
    ++(*this);
    return it;
}

inline const PackageIterator PackageIterator::operator--(int)
{
    PackageIterator it = *this;
    --(*this);
    return it;
}

PackageIterator operator+(PackageIterator iter, int n);
PackageIterator operator-(PackageIterator iter, int n);

#endif // __QUICKED_PACKAGE_ITERATOR_H__

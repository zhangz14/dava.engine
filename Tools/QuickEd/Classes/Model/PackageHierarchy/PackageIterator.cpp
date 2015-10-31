/*==================================================================================
    Copyright (c) 2008, binaryzebra
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.
    * Neither the name of the binaryzebra nor the
    names of its contributors may be used to endorse or promote products
    derived from this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE binaryzebra AND CONTRIBUTORS "AS IS" AND
    ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL binaryzebra BE LIABLE FOR ANY
    DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
    ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
=====================================================================================*/


#include "PackageIterator.h"
#include "PackageNode.h"
#include "PackageBaseNode.h"

struct PackageIterator::IteratorData
{
    MatchFunction func = [](const PackageBaseNode*)->bool {return true;};
    PackageBaseNode *currentNode = nullptr;
    bool Accaptable() const;
    PackageBaseNode *Next() const;
    PackageBaseNode *Previous() const;
    void InitFromNode(PackageBaseNode *node);
    
private:
    mutable DAVA::int32 currentIndex = 0;
    mutable DAVA::Stack<DAVA::int32> parentIndexes;
};

PackageIterator::PackageIterator(const PackageIterator &it)
    :  d_ptr(new IteratorData(*it.d_ptr))
{
    DVASSERT(nullptr != d_ptr->func);
}

PackageIterator::PackageIterator(PackageNode *package, MatchFunction func_)
    : d_ptr(new IteratorData())
{
    DVASSERT(nullptr != package);
    DVASSERT(nullptr != func_);
    d_ptr->func = func_;
    d_ptr->currentNode = const_cast<PackageBaseNode*>(package->Get(0));
}

PackageIterator::PackageIterator(PackageBaseNode *node, MatchFunction func_)
: d_ptr(new IteratorData())
{
    DVASSERT(nullptr != node);
    DVASSERT(nullptr != func_);
    d_ptr->func = func_;
    d_ptr->InitFromNode(node);

}

PackageIterator::~PackageIterator() =  default;

bool PackageIterator::IsValid() const
{
    return d_ptr->currentNode != nullptr;
}

void PackageIterator::SetMatchFunction(MatchFunction func)
{
    d_ptr->func = func;
}

PackageIterator &PackageIterator::operator=(const PackageIterator &it)
{
    *d_ptr.get() = *it.d_ptr.get();
    return *this;
}

/*!
 The prefix ++ operator (++it) advances the iterator to the next matching item
 and returns a reference to the resulting iterator.
 Sets the current pointer to 0 if the current item is the last matching item.
 */

PackageIterator &PackageIterator::operator++()
{
    if (d_ptr->currentNode != nullptr)
    {
        do
        {
            d_ptr->currentNode = d_ptr->Next();
        }
        while (IsValid() && !d_ptr->Accaptable());
    }
    return *this;
}

PackageIterator &PackageIterator::operator+=(int n)
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

PackageIterator &PackageIterator::operator--()
{
    if (d_ptr->currentNode != nullptr)
    {
        do
        {
            d_ptr->currentNode = d_ptr->Previous();
        }
        while (IsValid() && !d_ptr->Accaptable());
    }
    return *this;
}

PackageIterator &PackageIterator::operator-=(int n)
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
    return d_ptr->currentNode;
}


bool PackageIterator::IteratorData::Accaptable() const
{
    return func(currentNode);
}

void PackageIterator::IteratorData::InitFromNode(PackageBaseNode *node)
{
    currentNode = node;
    PackageBaseNode *parent = node->GetParent();
    decltype(parentIndexes) parentIndexesReverse;
    
    while(nullptr != parent)
    {
        parentIndexesReverse.push(parent->GetIndex(node));
        node = parent;
        parent = parent->GetParent();
    }
    while(!parentIndexesReverse.empty())
    {
        parentIndexes.push(parentIndexesReverse.top());
        parentIndexesReverse.pop();
    }
    currentIndex = parentIndexes.top();
    parentIndexes.pop();
}

PackageBaseNode* PackageIterator::IteratorData::Next() const
{
    DVASSERT(nullptr != currentNode && "calling Next for invalid iterator");
    PackageBaseNode *next = nullptr;
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
        PackageBaseNode *parent = currentNode->GetParent();
        DVASSERT(nullptr != parent) //orphan without parent and childs
        DAVA::uint32 count = parent->GetCount();
        next = currentIndex < (count - 1) ? parent->Get(currentIndex + 1) : nullptr;
        while (next == nullptr)
        {
            // if we had no sibling walk up the parent and try the sibling of that
            parent = parent->GetParent();
            if(nullptr == parent)
            {
                DVASSERT(parentIndexes.empty());
                return nullptr;
            }
            DVASSERT(!parentIndexes.empty());
            currentIndex = parentIndexes.top();
            parentIndexes.pop();
            if(nullptr != parent)
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

PackageBaseNode* PackageIterator::IteratorData::Previous() const
{
    DVASSERT(nullptr != currentNode && "calling Previous for invalid iterator");
    
    PackageBaseNode *prev = nullptr;
    PackageBaseNode *parent = currentNode->GetParent();
    if(parent == nullptr)
    {
        DVASSERT(currentIndex == 0 && parentIndexes.empty());
        return nullptr;
    }
    // walk the previous sibling
    prev = currentIndex > 0 ? parent->Get(currentIndex - 1) : nullptr;
    if (nullptr != prev) {
        // Yes, we had a previous sibling but we need go down to the last leafnode.
        --currentIndex;
        while (nullptr != prev && prev->GetCount()) {
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
    while(iter.IsValid() && n++)
    {
        ++iter;
    }
    return iter;
}

PackageIterator operator-(PackageIterator iter, int n)
{
    while(iter.IsValid() && n--)
    {
        --iter;
    }
    return iter;
}

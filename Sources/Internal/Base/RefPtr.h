#pragma once

#include "Base/Any.h"
#include "Base/BaseObject.h"

namespace DAVA
{
/// reference pointer wrapper for BaseObject refcounted classes.
template <class T>
class RefPtr
{
public:
    RefPtr() = default;

    explicit RefPtr(T* p)
        : _ptr(p)
    {
    }

    /// reinitializes pointer without incrementing reference
    void Set(T* p)
    {
        T* tmp_ptr = _ptr;
        _ptr = p;

        if (tmp_ptr)
            tmp_ptr->Release();
    }

    ~RefPtr()
    {
        if (_ptr)
            _ptr->Release();
    }

    RefPtr(const RefPtr<T>& rp)
    {
        _ptr = rp._ptr;

        if (_ptr)
            _ptr->Retain();
    }

    template <class Other>
    RefPtr(const RefPtr<Other>& rp)
    {
        _ptr = rp.Get();

        if (_ptr)
            _ptr->Retain();
    }

    static RefPtr<T> ConstructWithRetain(T* p)
    {
        static_assert(std::is_base_of<BaseObject, T>::value, "RefPtr works only with classes, derived from BaseObject!");
        p->Retain();
        return RefPtr<T>(p);
    }

    T* Get() const
    {
        return _ptr;
    }

    bool Valid() const
    {
        return _ptr != 0;
    }

    RefPtr& operator=(const RefPtr& rp)
    {
        assign(rp);
        return *this;
    }

    template <class Other>
    RefPtr& operator=(const RefPtr<Other>& rp)
    {
        assign(rp);
        return *this;
    }

    RefPtr& operator=(T* ptr)
    {
        if (_ptr == ptr)
            return *this;

        T* tmp_ptr = _ptr;
        _ptr = ptr;

        if (_ptr)
            _ptr->Retain();

        if (tmp_ptr)
            tmp_ptr->Release();

        return *this;
    }

    bool operator<(const RefPtr& other) const
    {
        return _ptr < other._ptr;
    }

    /// implicit output conversion
    //operator T*() const { return _ptr; }

    T& operator*() const
    {
        return *_ptr;
    }

    T* operator->() const
    {
        return _ptr;
    }

    bool operator==(const RefPtr& rp) const
    {
        return _ptr == rp._ptr;
    }
    bool operator==(const T* ptr) const
    {
        return _ptr == ptr;
    }
    friend bool operator==(const T* ptr, const RefPtr& rp)
    {
        return ptr == rp._ptr;
    }
    bool operator!=(const RefPtr& rp) const
    {
        return _ptr != rp._ptr;
    }
    bool operator!=(const T* ptr) const
    {
        return _ptr != ptr;
    }
    friend bool operator!=(const T* ptr, const RefPtr& rp)
    {
        return ptr != rp._ptr;
    }
    bool operator!() const // Enables "if (!sp) ..."
    {
        return _ptr == nullptr;
    }

    template <typename... Arg>
    void ConstructInplace(Arg&&... arg)
    {
        Set(new T(std::forward<Arg>(arg)...));
    }

    template <typename... Arg>
    static RefPtr<T> Construct(Arg&&... arg)
    {
        return RefPtr<T>(new T(std::forward<Arg>(arg)...));
    }

private:
    class Tester
    {
        void operator delete(void*);
    };

public:
    operator Tester*() const
    {
        if (_ptr == nullptr)
            return nullptr;

        static Tester test;
        return &test;
    }

private:
    T* _ptr = nullptr;

    template <class Other>
    void assign(const RefPtr<Other>& rp)
    {
        if (_ptr == rp.Get())
            return;

        T* tmp_ptr = _ptr;
        _ptr = rp.Get();

        if (_ptr)
            _ptr->Retain();

        if (tmp_ptr)
            tmp_ptr->Release();
    }
};

template <typename T>
struct AnyCompare<RefPtr<T>>
{
    static bool IsEqual(const Any& v1, const Any& v2)
    {
        const RefPtr<T>& s1 = v1.Get<RefPtr<T>>();
        const RefPtr<T>& s2 = v2.Get<RefPtr<T>>();
        return s1 == s2;
    }
};
}

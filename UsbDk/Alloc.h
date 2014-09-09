/**********************************************************************
* Copyright (c) 2013-2014  Red Hat, Inc.
*
* Developed by Daynix Computing LTD.
*
* Authors:
*     Dmitry Fleytman <dmitry@daynix.com>
*     Pavel Gurvich <pavel@daynix.com>
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
**********************************************************************/

#pragma once

template <POOL_TYPE PoolType, ULONG Tag>
class CAllocatable
{
public:
    void* operator new(size_t /* size */, void *ptr) throw()
        { return ptr; }

    void* operator new[](size_t /* size */, void *ptr) throw()
        { return ptr; }

    void* operator new(size_t Size) throw()
        { return ExAllocatePoolWithTag(PoolType, Size, Tag); }

    void* operator new[](size_t Size) throw()
        { return ExAllocatePoolWithTag(PoolType, Size, Tag); }

    void operator delete(void *ptr)
        { if(ptr) { ExFreePoolWithTag(ptr, Tag); } }

    void operator delete[](void *ptr)
        { if(ptr) { ExFreePoolWithTag(ptr, Tag); } }

protected:
    CAllocatable() {};
    ~CAllocatable() {};
};

template<typename T>
class CObjHolder
{
public:
    typedef void(*TDeleteFunc)(T *Obj);

    CObjHolder(T *Obj = nullptr, TDeleteFunc DeleteFunc = [](T *Obj){ delete Obj; })
        : m_Obj(Obj)
        , m_DeleteFunc(DeleteFunc)
    {}

    ~CObjHolder()
    { destroy(); }

    operator bool() const { return m_Obj != nullptr; }
    operator T *() const { return m_Obj; }
    T *operator ->() const { return m_Obj; }

    T *detach()
    {
        auto ptr = m_Obj;
        m_Obj = nullptr;
        return ptr;
    }

    void destroy()
    {
        if (*this)
        {
            m_DeleteFunc(m_Obj);
        }
    }

    T* operator= (T *ptr)
    {
      m_Obj = ptr;
      return ptr;
    }

    static void ArrayHolderDelete(T *Obj)
    { delete[] Obj; }

    CObjHolder(const CObjHolder&) = delete;
    CObjHolder& operator= (const CObjHolder&) = delete;

private:
    T *m_Obj = nullptr;
    TDeleteFunc m_DeleteFunc;
};

template<typename T>
class CRefCountingHolder : public CAllocatable < NonPagedPool, 'CRHR'>
{
public:
    typedef void(*TDeleteFunc)(T *Obj);

    CRefCountingHolder(TDeleteFunc DeleteFunc = [](T *Obj){ delete Obj; })
        : m_DeleteFunc(DeleteFunc)
    {};

    bool InitialAddRef()
    {
        return InterlockedIncrement(&m_RefCount) == 1;
    }

    void AddRef()
    {
        InterlockedIncrement(&m_RefCount);
    }

    void Release()
    {
        if ((InterlockedDecrement(&m_RefCount) == 0) && (m_Obj != nullptr))
        {
            m_DeleteFunc(m_Obj);
        }
    }

    operator T *() { return m_Obj; }
    T *Get() { return m_Obj; }
    T *operator ->() { return m_Obj; }

    void operator= (T *ptr)
    {
        m_Obj = ptr;
    }

    CRefCountingHolder(const CRefCountingHolder&) = delete;
    CRefCountingHolder& operator= (const CRefCountingHolder&) = delete;

private:
    T *m_Obj = nullptr;
    LONG m_RefCount = 0;
    TDeleteFunc m_DeleteFunc;
};

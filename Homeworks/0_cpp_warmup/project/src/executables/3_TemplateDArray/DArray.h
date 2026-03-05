#pragma once

#include <iostream>

// interfaces of Dynamic Array class DArray
template<class T>
class DArray {
public:
    DArray();
    DArray(int nSize, const T& dValue = static_cast<T>(0));
    DArray(const DArray& arr);
    ~DArray();

    void Print() const;

    int GetSize() const;
    void SetSize(int nSize);

    const T& GetAt(int nIndex) const;
    void SetAt(int nIndex, const T& dValue);

    T& operator[](int nIndex);
    const T& operator[](int nIndex) const;

    void PushBack(const T& dValue);
    void DeleteAt(int nIndex);
    void InsertAt(int nIndex, const T& dValue);

    DArray& operator=(const DArray& arr);

private:
    T* m_pData;
    int m_nSize;
    int m_nMax;

private:
    void Init();
    void Free();
    void Reserve(int nSize);
};

#include "DArray.inl"

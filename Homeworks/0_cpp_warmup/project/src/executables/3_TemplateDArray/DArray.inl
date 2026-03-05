template<class T>
DArray<T>::DArray() {
    Init();
}

template<class T>
DArray<T>::DArray(int nSize, const T& dValue) {
    Init();

    if (nSize < 0) {
        std::cerr << "Error: size of array should be non-negative." << std::endl;
        return;
    }
    if (nSize == 0) {
        return;
    }

    m_nSize = nSize;
    m_nMax = nSize;
    m_pData = new T[m_nMax];
    for (int i = 0; i < m_nSize; ++i) {
        m_pData[i] = dValue;
    }
}

template<class T>
DArray<T>::DArray(const DArray& arr) {
    Init();

    if (arr.m_nSize == 0) {
        return;
    }

    m_nSize = arr.m_nSize;
    m_nMax = arr.m_nMax;
    m_pData = new T[m_nMax];
    for (int i = 0; i < m_nSize; ++i) {
        m_pData[i] = arr.m_pData[i];
    }
}

template<class T>
DArray<T>::~DArray() {
    Free();
}

template<class T>
void DArray<T>::Print() const {
    for (int i = 0; i < m_nSize; ++i) {
        std::cout << m_pData[i] << " ";
    }
    std::cout << std::endl;
}

template<class T>
void DArray<T>::Init() {
    m_pData = nullptr;
    m_nSize = 0;
    m_nMax = 0;
}

template<class T>
void DArray<T>::Free() {
    if (m_pData != nullptr) {
        delete[] m_pData;
        m_pData = nullptr;
    }
    m_nSize = 0;
    m_nMax = 0;
}

template<class T>
void DArray<T>::Reserve(int nSize) {
    if (nSize <= m_nMax) {
        return;
    }

    T* pNewData = new T[nSize];
    for (int i = 0; i < m_nSize; ++i) {
        pNewData[i] = m_pData[i];
    }

    delete[] m_pData;
    m_pData = pNewData;
    m_nMax = nSize;
}

template<class T>
int DArray<T>::GetSize() const {
    return m_nSize;
}

template<class T>
void DArray<T>::SetSize(int nSize) {
    if (nSize < 0) {
        std::cerr << "Error: size of array should be non-negative." << std::endl;
        return;
    }

    if (nSize == 0) {
        Free();
        return;
    }

    if (nSize > m_nMax) {
        Reserve(nSize);
    }

    if (nSize > m_nSize) {
        for (int i = m_nSize; i < nSize; ++i) {
            m_pData[i] = T();
        }
    }

    m_nSize = nSize;
}

template<class T>
const T& DArray<T>::GetAt(int nIndex) const {
    if (nIndex < 0 || nIndex >= m_nSize) {
        std::cerr << "Error: index out of range. Return default value." << std::endl;
        static T s_error = T();
        return s_error;
    }

    return m_pData[nIndex];
}

template<class T>
void DArray<T>::SetAt(int nIndex, const T& dValue) {
    if (nIndex < 0 || nIndex >= m_nSize) {
        std::cerr << "Error: index out of range." << std::endl;
        return;
    }

    m_pData[nIndex] = dValue;
}

template<class T>
T& DArray<T>::operator[](int nIndex) {
    return const_cast<T&>(static_cast<const DArray&>(*this).GetAt(nIndex));
}

template<class T>
const T& DArray<T>::operator[](int nIndex) const {
    return GetAt(nIndex);
}

template<class T>
void DArray<T>::PushBack(const T& dValue) {
    if (m_nSize == m_nMax) {
        Reserve(m_nMax * 2 + 1);
    }

    m_pData[m_nSize] = dValue;
    ++m_nSize;
}

template<class T>
void DArray<T>::DeleteAt(int nIndex) {
    if (nIndex < 0 || nIndex >= m_nSize) {
        std::cerr << "Error: index out of range." << std::endl;
        return;
    }

    for (int i = nIndex; i < m_nSize - 1; ++i) {
        m_pData[i] = m_pData[i + 1];
    }
    --m_nSize;
}

template<class T>
void DArray<T>::InsertAt(int nIndex, const T& dValue) {
    if (nIndex < 0 || nIndex > m_nSize) {
        std::cerr << "Error: index out of range." << std::endl;
        return;
    }

    if (m_nSize == m_nMax) {
        Reserve(m_nMax * 2 + 1);
    }

    for (int i = m_nSize; i > nIndex; --i) {
        m_pData[i] = m_pData[i - 1];
    }
    m_pData[nIndex] = dValue;
    ++m_nSize;
}

template<class T>
DArray<T>& DArray<T>::operator=(const DArray& arr) {
    if (this == &arr) {
        return *this;
    }

    if (arr.m_nSize == 0) {
        Free();
        return *this;
    }

    Free();
    m_nSize = arr.m_nSize;
    m_nMax = arr.m_nMax;
    m_pData = new T[m_nMax];
    for (int i = 0; i < m_nSize; ++i) {
        m_pData[i] = arr.m_pData[i];
    }

    return *this;
}

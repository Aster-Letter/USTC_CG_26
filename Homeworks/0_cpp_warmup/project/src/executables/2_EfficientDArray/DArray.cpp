// implementation of class DArray
#include "DArray.h"

#include <iostream>

namespace {
double& InvalidValue() {
	static double s_invalid_value = 0.0;
	return s_invalid_value;
}
}

// default constructor
DArray::DArray() {
	Init();
}

// set an array with default values
DArray::DArray(int nSize, double dValue) {
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
	m_pData = new double[m_nMax];
	for (int i = 0; i < m_nSize; ++i) {
		m_pData[i] = dValue;
	}
}

DArray::DArray(const DArray& arr) {
	Init();
	if (arr.m_nSize == 0) {
		return;
	}

	m_nSize = arr.m_nSize;
	m_nMax = arr.m_nMax;
	m_pData = new double[m_nMax];
	for (int i = 0; i < m_nSize; ++i) {
		m_pData[i] = arr.m_pData[i];
	}
}

// deconstructor
DArray::~DArray() {
	Free();
}

// display the elements of the array
void DArray::Print() const {
	for (int i = 0; i < m_nSize; ++i) {
		std::cout << m_pData[i] << " ";
	}
	std::cout << std::endl;
}

// initilize the array
void DArray::Init() {
	m_pData = nullptr;
	m_nSize = 0;
	m_nMax = 0;
}

// free the array
void DArray::Free() {
	delete[] m_pData;
	m_pData = nullptr;
	m_nSize = 0;
	m_nMax = 0;
}

// get the size of the array
int DArray::GetSize() const {
	return m_nSize;
}

// set the size of the array
void DArray::SetSize(int nSize) {
	if (nSize < 0) {
		std::cerr << "Error: size of array should be non-negative." << std::endl;
		return;
	}

	if (nSize == 0) {
		Free();
		return;
	}

	if (nSize > m_nMax) {
		int nNewCapacity = (m_nMax > 0) ? m_nMax : 1;
		while (nNewCapacity < nSize) {
			nNewCapacity = nNewCapacity * 2;
		}
		Reserve(nNewCapacity);
	}

	for (int i = m_nSize; i < nSize; ++i) {
		m_pData[i] = 0.0;
	}
	m_nSize = nSize;
}

// get an element at an index
const double& DArray::GetAt(int nIndex) const {
	if (nIndex < 0 || nIndex >= m_nSize) {
		std::cerr << "Error: index out of range. Return default value." << std::endl;
		return InvalidValue();
	}
	return m_pData[nIndex];
}

// set the value of an element
void DArray::SetAt(int nIndex, double dValue) {
	if (nIndex < 0 || nIndex >= m_nSize) {
		std::cerr << "Error: index out of range." << std::endl;
		return;
	}
	m_pData[nIndex] = dValue;
}

// overload operator '[]'
double& DArray::operator[](int nIndex) {
	if (nIndex < 0 || nIndex >= m_nSize) {
		std::cerr << "Error: index out of range. Return default value." << std::endl;
		return InvalidValue();
	}
	return m_pData[nIndex];
}

// overload operator '[]'
const double& DArray::operator[](int nIndex) const {
	return GetAt(nIndex);
}

// add a new element at the end of the array
void DArray::PushBack(double dValue) {
	if (m_nSize == m_nMax) {
		Reserve((m_nMax > 0) ? (m_nMax * 2) : 1);
	}
	m_pData[m_nSize] = dValue;
	++m_nSize;
}

// delete an element at some index
void DArray::DeleteAt(int nIndex) {
	if (nIndex < 0 || nIndex >= m_nSize) {
		std::cerr << "Error: index out of range." << std::endl;
		return;
	}
	for (int i = nIndex; i < m_nSize - 1; ++i) {
		m_pData[i] = m_pData[i + 1];
	}
	--m_nSize;
}

// insert a new element at some index
void DArray::InsertAt(int nIndex, double dValue) {
	if (nIndex < 0 || nIndex > m_nSize) {
		std::cerr << "Error: index out of range." << std::endl;
		return;
	}

	if (m_nSize == m_nMax) {
		Reserve((m_nMax > 0) ? (m_nMax * 2) : 1);
	}
	for (int i = m_nSize; i > nIndex; --i) {
		m_pData[i] = m_pData[i - 1];
	}
	m_pData[nIndex] = dValue;
	++m_nSize;
}

// overload operator '='
DArray& DArray::operator=(const DArray& arr) {
	if (this == &arr) {
		return *this;
	}
	if (arr.m_nSize == 0) {
		Free();
		return *this;
	}

	double* pNewData = new double[arr.m_nMax];
	for (int i = 0; i < arr.m_nSize; ++i) {
		pNewData[i] = arr.m_pData[i];
	}

	delete[] m_pData;
	m_pData = pNewData;
	m_nSize = arr.m_nSize;
	m_nMax = arr.m_nMax;
	return *this;
}

// allocate enough memory
void DArray::Reserve(int nSize) {
	if (nSize <= m_nMax) {
		return;
	}

	double* pNewData = new double[nSize];
	for (int i = 0; i < m_nSize; ++i) {
		pNewData[i] = m_pData[i];
	}
	delete[] m_pData;
	m_pData = pNewData;
	m_nMax = nSize;
}
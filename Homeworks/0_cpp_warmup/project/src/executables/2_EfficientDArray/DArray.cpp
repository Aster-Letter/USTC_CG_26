// implementation of class DArray
#include "DArray.h"
#include <iostream>

// default constructor
DArray::DArray() {
	Init();
}

// set an array with default values
DArray::DArray(int nSize, double dValue) {
	Init();
	//Error
	if (nSize < 0) {
		std::cerr << "Error: size of array should be non-negative." << std::endl;
		return;
	}
	//Special: 0
	if (nSize == 0) {
		return;
	}

	m_nSize = nSize;
	m_nMax = nSize;
	m_pData = new double[m_nMax];
	for (int i = 0; i < m_nSize; i++) {
		m_pData[i] = dValue;
	}
}

DArray::DArray(const DArray& arr) {
	Init();
	//Special:empty copy
	if (arr.m_nSize == 0) {
		return;
	}

	m_nSize = arr.m_nSize;
	m_nMax = arr.m_nMax;
	m_pData = new double[m_nMax];
	for (int i = 0; i < m_nSize; i++) {
		m_pData[i] = arr.m_pData[i];
	}
}

// deconstructor
DArray::~DArray() {
	Free();
}

// display the elements of the array
void DArray::Print() const {
	for (int i = 0; i < m_nSize; i++) {
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
	if (m_pData != nullptr) {
		delete[] m_pData;
		m_pData = nullptr;
	}
	m_nSize = 0;
	m_nMax = 0;
}

// get the size of the array
int DArray::GetSize() const {
	return m_nSize; 
}

// set the size of the array
void DArray::SetSize(int nSize) {
	//Error: negative
	if (nSize < 0) {
		std::cerr << "Error: size of array should be non-negative." << std::endl;
		return;
	}

	//Special: 0
	if (nSize == 0){
		Free();
		return;
	}

	if (nSize > m_nMax) {
		Reserve(nSize);
	}

	if (nSize <= m_nSize) {
		m_nSize = nSize;
	}
	else {
		for (int i = m_nSize; i < nSize; i++) {
			m_pData[i] = 0;
		}
		m_nSize = nSize;
	}
}

// get an element at an index
const double& DArray::GetAt(int nIndex) const {
	//Error: out of range
	if (nIndex < 0 || nIndex >= m_nSize) {
		std::cerr << "Error: index out of range. Return default value." << std::endl;
		static double error = 0.0;
		return error;
	}
	return m_pData[nIndex];
}

// set the value of an element 
void DArray::SetAt(int nIndex, double dValue) {
	//Error: out of range
	if (nIndex < 0 || nIndex >= m_nSize) {
		std::cerr << "Error: index out of range." << std::endl;
		return;
	}
	m_pData[nIndex] = dValue;
}

// overload operator '[]'
double& DArray::operator[](int nIndex) {
	return const_cast<double&>(static_cast<const DArray&>(*this).GetAt(nIndex));//Assisted by AI.
}

// overload operator '[]'
const double& DArray::operator[](int nIndex) const {
	return GetAt(nIndex);
}

// add a new element at the end of the array
void DArray::PushBack(double dValue) {
	if (m_nSize == m_nMax) {
		Reserve(m_nMax * 2 + 1);
	}
	m_pData[m_nSize] = dValue;
	m_nSize++;
}

// delete an element at some index
void DArray::DeleteAt(int nIndex) {
	//Error: out of range
	if (nIndex < 0 || nIndex >= m_nSize) {
		std::cerr << "Error: index out of range." << std::endl;
		return;
	}
	for (int i = nIndex; i < m_nSize - 1; i++) {
		m_pData[i] = m_pData[i + 1];
	}
	m_nSize--;
}

// insert a new element at some index
void DArray::InsertAt(int nIndex, double dValue) {
	//Error: out of range
	if (nIndex < 0 || nIndex > m_nSize) {
		std::cerr << "Error: index out of range." << std::endl;
		return;
	}

	if (m_nSize == m_nMax) {
		Reserve(m_nMax * 2 + 1);
	}
	for (int i = m_nSize; i > nIndex; i--) {
		m_pData[i] = m_pData[i - 1];
	}
	m_pData[nIndex] = dValue;
	m_nSize++;
}


// overload operator '='
DArray& DArray::operator = (const DArray& arr) {
	//Special: self assignment
	if (this == &arr) {
		return *this;
	}
	//Special: empty copy
	if (arr.m_nSize == 0) {
		Free();
		return *this;
	}
	
	Free();
	m_nSize = arr.m_nSize;
	m_nMax = arr.m_nMax;
	m_pData = new double[m_nMax];
	for (int i = 0; i < m_nSize; i++) {
		m_pData[i] = arr.m_pData[i];
	}
	return *this;
}

// allocate enough memory
void DArray::Reserve(int nSize) {
	// No need for Error or Special, Since privacy.
	// Default: nSize > m_nMax
	double* pNewData = new double[nSize];
	for (int i = 0; i < m_nSize; i++) {
		pNewData[i] = m_pData[i];
	}
	delete[] m_pData;
	m_pData = pNewData;
	m_nMax = nSize;
}
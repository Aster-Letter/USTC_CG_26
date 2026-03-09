#include "PolynomialMap.h"

#include <iostream>
#include <fstream>
#include <cassert>
#include <cmath>

using namespace std;

PolynomialMap::PolynomialMap(const PolynomialMap& other) {
	// Special: empty
	if (other.m_Polynomial.empty()) {
		return;
	}

	for (const auto& term : other.m_Polynomial) {
		m_Polynomial[term.first] = term.second;
	}
}

PolynomialMap::PolynomialMap(const string& file) {
    ReadFromFile(file);
	return;
}

PolynomialMap::PolynomialMap(const double* cof, const int* deg, int n) {
	// Special: negative n
	if (n < 0) {
		cout << "Negative n: " << n << endl;
		return;
	}
	// Special: negative degree
	for (int i = 0; i < n; ++i) {
		if (deg[i] < 0) {
			cout << "Negative degree: " << deg[i] << endl;
			return;
		}
	}
	for (int i = 0; i < n; ++i) {
		m_Polynomial[deg[i]] += cof[i];
	}
}

PolynomialMap::PolynomialMap(const vector<int>& deg, const vector<double>& cof) {
	assert(deg.size() == cof.size());
	// Special: negative degree
	for (size_t i = 0; i < deg.size(); ++i) {
		if (deg[i] < 0) {
			cout << "Negative degree: " << deg[i] << endl;
			return;
		}
	}
	for (size_t i = 0; i < deg.size(); ++i) {
		m_Polynomial[deg[i]] += cof[i];
	}
}

double PolynomialMap::coff(int i) const {
	auto it = m_Polynomial.find(i);
	if (it != m_Polynomial.end()) {
		return it->second;
	}
	return 0.f;
}

double& PolynomialMap::coff(int i) {
	return m_Polynomial[i]; // default value is 0.f
}

void PolynomialMap::compress() {
	for (auto it = m_Polynomial.begin(); it != m_Polynomial.end();) {
		if (std::abs(it->second) < 1e-8) {
			it = m_Polynomial.erase(it);
		} else {
			++it;
		}
	}
}

PolynomialMap PolynomialMap::operator+(const PolynomialMap& right) const {
	PolynomialMap result(*this);
	for (const auto& term : right.m_Polynomial) {
		result.m_Polynomial[term.first] += term.second;
	}
	result.compress();
	return result;
}

PolynomialMap PolynomialMap::operator-(const PolynomialMap& right) const {
	PolynomialMap result(*this);
	for (const auto& term : right.m_Polynomial) {
		result.m_Polynomial[term.first] -= term.second;
	}
	result.compress();
	return result;
}

PolynomialMap PolynomialMap::operator*(const PolynomialMap& right) const {
	PolynomialMap result;
	for (const auto& term1 : m_Polynomial) {
		for (const auto& term2 : right.m_Polynomial) {
			int deg = term1.first + term2.first;
			double cof = term1.second * term2.second;
			result.m_Polynomial[deg] += cof;
		}
	}
	result.compress();
	return result;
}

PolynomialMap& PolynomialMap::operator=(const PolynomialMap& right) {
	m_Polynomial = right.m_Polynomial;
	return *this;
}

void PolynomialMap::Print() const {
	// Special: empty
	if (m_Polynomial.empty()) {
		cout << "0" << endl;
		return;
	}
	for (auto& term = m_Polynomial.rbegin(); term != m_Polynomial.rend(); ++term) {
		cout << term->second << "x^" << term->first << " ";
	}
	cout << endl;
}

bool PolynomialMap::ReadFromFile(const string& file) {
    m_Polynomial.clear();
	ifstream fin(file);
	if (!fin.is_open()) {
		cout << "Failed to open file: " << file << endl;
		return false;
	}
	int deg;
	double cof;
	int num = 0;
	char check;
	if (!(fin >> check >> num) || check != 'P') {
		cout << "Invalid file format: " << file << endl;
		return false;
	}
	for (int i = 0; i < num; ++i) {
		fin >> deg >> cof;
		m_Polynomial[deg] += cof;
	}

	return true; // you should return a correct value
}

#include "PolynomialMap.h"

#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>

namespace {
constexpr double kEpsilon = 1e-8;

bool IsZero(double value) {
	return std::abs(value) < kEpsilon;
}

double& InvalidCoefficient() {
	static double s_invalid_coefficient = 0.0;
	return s_invalid_coefficient;
}

std::ifstream OpenPolynomialFile(const std::string& file, std::filesystem::path* resolved_path) {
	namespace fs = std::filesystem;

	const fs::path input_path(file);
	std::vector<fs::path> candidates;
	candidates.push_back(input_path);

	const fs::path filename = input_path.filename();
	if (!filename.empty()) {
		candidates.push_back(fs::path("data") / filename);
		candidates.push_back(fs::path("..") / "data" / filename);
		candidates.push_back(fs::path("project") / "data" / filename);
	}

	for (const auto& candidate : candidates) {
		std::ifstream fin(candidate);
		if (fin.is_open()) {
			if (resolved_path != nullptr) {
				*resolved_path = candidate;
			}
			return fin;
		}
	}

	if (resolved_path != nullptr) {
		*resolved_path = input_path;
	}
	return std::ifstream();
}
}

PolynomialMap::PolynomialMap(const PolynomialMap& other)
	: m_Polynomial(other.m_Polynomial) {
}

PolynomialMap::PolynomialMap(const std::string& file) {
	ReadFromFile(file);
}

PolynomialMap::PolynomialMap(const double* cof, const int* deg, int n) {
	if (n < 0) {
		std::cerr << "Error: polynomial term count should be non-negative." << std::endl;
		return;
	}
	if (n > 0 && (cof == nullptr || deg == nullptr)) {
		std::cerr << "Error: polynomial input arrays should not be null." << std::endl;
		return;
	}

	for (int i = 0; i < n; ++i) {
		if (deg[i] < 0) {
			std::cerr << "Warning: skip negative degree " << deg[i] << std::endl;
			continue;
		}
		m_Polynomial[deg[i]] += cof[i];
	}
	compress();
}

PolynomialMap::PolynomialMap(const std::vector<int>& deg, const std::vector<double>& cof) {
	const std::size_t nSize = (deg.size() < cof.size()) ? deg.size() : cof.size();
	if (deg.size() != cof.size()) {
		std::cerr << "Warning: degree/coefficient vector sizes do not match."
		          << " Only the overlapping prefix is used." << std::endl;
	}

	for (std::size_t i = 0; i < nSize; ++i) {
		if (deg[i] < 0) {
			std::cerr << "Warning: skip negative degree " << deg[i] << std::endl;
			continue;
		}
		m_Polynomial[deg[i]] += cof[i];
	}
	compress();
}

double PolynomialMap::coff(int i) const {
	if (i < 0) {
		return 0.0;
	}

	auto it = m_Polynomial.find(i);
	if (it != m_Polynomial.end()) {
		return it->second;
	}
	return 0.0;
}

double& PolynomialMap::coff(int i) {
	if (i < 0) {
		std::cerr << "Error: polynomial degree should be non-negative." << std::endl;
		return InvalidCoefficient();
	}
	return m_Polynomial[i];
}

void PolynomialMap::compress() {
	for (auto it = m_Polynomial.begin(); it != m_Polynomial.end();) {
		if (IsZero(it->second)) {
			it = m_Polynomial.erase(it);
		}
		else {
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
			result.m_Polynomial[term1.first + term2.first] += term1.second * term2.second;
		}
	}
	result.compress();
	return result;
}

PolynomialMap& PolynomialMap::operator=(const PolynomialMap& right) {
	if (this != &right) {
		m_Polynomial = right.m_Polynomial;
	}
	return *this;
}

void PolynomialMap::Print() const {
	if (m_Polynomial.empty()) {
		std::cout << "0" << std::endl;
		return;
	}
	for (auto it = m_Polynomial.rbegin(); it != m_Polynomial.rend(); ++it) {
		std::cout << it->second << "x^" << it->first << " ";
	}
	std::cout << std::endl;
}

bool PolynomialMap::ReadFromFile(const std::string& file) {
	m_Polynomial.clear();

	std::filesystem::path resolved_path;
	std::ifstream fin = OpenPolynomialFile(file, &resolved_path);
	if (!fin.is_open()) {
		std::cerr << "Error: failed to open file " << file << std::endl;
		return false;
	}

	char check = 0;
	int num = 0;
	if (!(fin >> check >> num) || check != 'P' || num < 0) {
		std::cerr << "Error: invalid polynomial file header in " << resolved_path.string() << std::endl;
		return false;
	}

	for (int i = 0; i < num; ++i) {
		int deg = 0;
		double cof = 0.0;
		if (!(fin >> deg >> cof)) {
			std::cerr << "Error: invalid polynomial term data in " << resolved_path.string() << std::endl;
			m_Polynomial.clear();
			return false;
		}
		if (deg < 0) {
			std::cerr << "Warning: skip negative degree " << deg << " in " << resolved_path.string() << std::endl;
			continue;
		}
		m_Polynomial[deg] += cof;
	}

	compress();
	return true;
}

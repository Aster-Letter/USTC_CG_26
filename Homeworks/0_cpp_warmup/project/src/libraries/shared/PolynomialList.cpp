#include "PolynomialList.h"

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

PolynomialList::PolynomialList(const PolynomialList& other)
    : m_Polynomial(other.m_Polynomial) {
}

PolynomialList::PolynomialList(const std::string& file) {
    ReadFromFile(file);
}

PolynomialList::PolynomialList(const double* cof, const int* deg, int n) {
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
        AddOneTerm(Term(deg[i], cof[i]));
    }
    compress();
}

PolynomialList::PolynomialList(const std::vector<int>& deg, const std::vector<double>& cof) {
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
        AddOneTerm(Term(deg[i], cof[i]));
    }
    compress();
}

double PolynomialList::coff(int i) const {
    if (i < 0) {
        return 0.0;
    }

    for (const auto& term : m_Polynomial) {
        if (term.deg == i) {
            return term.cof;
        }
    }
    return 0.0;
}

double& PolynomialList::coff(int i) {
    if (i < 0) {
        std::cerr << "Error: polynomial degree should be non-negative." << std::endl;
        return InvalidCoefficient();
    }

    for (auto& term : m_Polynomial) {
        if (term.deg == i) {
            return term.cof;
        }
    }

    return AddOneTerm(Term(i, 0.0)).cof;
}

void PolynomialList::compress() {
    for (auto it = m_Polynomial.begin(); it != m_Polynomial.end();) {
        if (IsZero(it->cof)) {
            it = m_Polynomial.erase(it);
        }
        else {
            ++it;
        }
    }
}

PolynomialList PolynomialList::operator+(const PolynomialList& right) const {
    PolynomialList result(*this);
    for (const auto& term : right.m_Polynomial) {
        result.AddOneTerm(term);
    }
    result.compress();
    return result;
}

PolynomialList PolynomialList::operator-(const PolynomialList& right) const {
    PolynomialList result(*this);
    for (const auto& term : right.m_Polynomial) {
        result.AddOneTerm(Term(term.deg, -term.cof));
    }
    result.compress();
    return result;
}

PolynomialList PolynomialList::operator*(const PolynomialList& right) const {
    PolynomialList result;
    for (const auto& term1 : m_Polynomial) {
        for (const auto& term2 : right.m_Polynomial) {
            result.AddOneTerm(Term(term1.deg + term2.deg, term1.cof * term2.cof));
        }
    }
    result.compress();
    return result;
}

PolynomialList& PolynomialList::operator=(const PolynomialList& right) {
    if (this != &right) {
        m_Polynomial = right.m_Polynomial;
    }
    return *this;
}

void PolynomialList::Print() const {
    if (m_Polynomial.empty()) {
        std::cout << "0" << std::endl;
        return;
    }

    for (const auto& term : m_Polynomial) {
        std::cout << term.cof << "x^" << term.deg << " ";
    }
    std::cout << std::endl;
}

bool PolynomialList::ReadFromFile(const std::string& file) {
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
        AddOneTerm(Term(deg, cof));
    }

    compress();
    return true;
}

PolynomialList::Term& PolynomialList::AddOneTerm(const Term& term) {
    for (auto it = m_Polynomial.begin(); it != m_Polynomial.end(); ++it) {
        if (it->deg == term.deg) {
            it->cof += term.cof;
            return *it;
        }
        if (it->deg < term.deg) {
            auto inserted = m_Polynomial.insert(it, term);
            return *inserted;
        }
    }

    m_Polynomial.push_back(term);
    return m_Polynomial.back();
}

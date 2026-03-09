#include "PolynomialList.h"
#include <list>
#include <fstream>
#include <iostream>
#include <cmath>

using namespace std;

PolynomialList::PolynomialList(const PolynomialList& other) {
    // Special: empty
    if (other.m_Polynomial.empty()) {
        return;
    }

    for (const auto& term : other.m_Polynomial) {
        m_Polynomial.push_back(term);
    }

}

PolynomialList::PolynomialList(const string& file) {
    ReadFromFile(file);
    return;
}

PolynomialList::PolynomialList(const double* cof, const int* deg, int n) {
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
        AddOneTerm(Term(deg[i], cof[i]));
    }
    return;
}

PolynomialList::PolynomialList(const vector<int>& deg, const vector<double>& cof) {
    // Special: negative degree
    for (size_t i = 0; i < deg.size(); ++i) {
        if (deg[i] < 0) {
            cout << "Negative degree: " << deg[i] << endl;
            return;
        }
    }
    for (size_t i = 0; i < deg.size(); ++i) {
        AddOneTerm(Term(deg[i], cof[i]));
    }
    return;
}

double PolynomialList::coff(int i) const {
    for (const auto& term : m_Polynomial) {
        if (term.deg == i) {
            return term.cof;
        }
    }
    return 0.f; // default value
}

double& PolynomialList::coff(int i) {
    for (auto& term : m_Polynomial) {
        if (term.deg == i) {
            return term.cof;
        }
    }
    // If not found, add a new term with degree i and coefficient 0
    return AddOneTerm(Term(i, 0)).cof;
}

void PolynomialList::compress() {
    for (auto it = m_Polynomial.begin(); it != m_Polynomial.end();) {
        if (std::abs(it->cof) < 1e-8) {
            it = m_Polynomial.erase(it);
        } else {
            ++it;
        }
    }
}

PolynomialList PolynomialList::operator+(const PolynomialList& right) const {
    // Special: empty
    if (right.m_Polynomial.empty()) {
        return *this;
    }
    PolynomialList result(*this);
    for (const auto& term : right.m_Polynomial) {
        result.AddOneTerm(term);
    }
    result.compress();
    return result;
}

PolynomialList PolynomialList::operator-(const PolynomialList& right) const {
    // Special: empty
    if (right.m_Polynomial.empty()) {
        return *this;
    }

    PolynomialList result(*this);
    for (const auto& term : right.m_Polynomial) {
        result.AddOneTerm(Term(term.deg, -term.cof));
    }
    result.compress();
    return result;
}

PolynomialList PolynomialList::operator*(const PolynomialList& right) const {
    // Special: empty
    if (right.m_Polynomial.empty() || m_Polynomial.empty()) {
        return PolynomialList();
    }

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
    if (this == &right) {
        return *this;
    }
    m_Polynomial.clear();
    for (const auto& term : right.m_Polynomial) {
        m_Polynomial.push_back(term);
    }
    return *this;
}

void PolynomialList::Print() const {
    // Special: empty
    if (m_Polynomial.empty()) {
        cout << "0" << endl;
        return;
    }
    for (const auto& term : m_Polynomial) {
        cout << term.cof << "x^" << term.deg << " ";
    }
    cout << endl;
}

bool PolynomialList::ReadFromFile(const string& file) {
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
    if (!(fin >> check >> num)) {
        cout << "Empty file: " << file << endl;
        return false;
    }

    for (int i = 0; i < num; ++i) {
        fin >> deg >> cof;
        AddOneTerm(Term(deg, cof));
    }

    return true; 
}

PolynomialList::Term& PolynomialList::AddOneTerm(const Term& term) {
    for (auto& t = m_Polynomial.begin(); t != m_Polynomial.end(); t++) {
        if (t->deg == term.deg) {
            t->cof += term.cof;
            return *t;
        }
        if (t->deg < term.deg) {
            m_Polynomial.insert(t, term);
            return *prev(t);
        }
    }
    m_Polynomial.push_back(term);
    return m_Polynomial.back();
}

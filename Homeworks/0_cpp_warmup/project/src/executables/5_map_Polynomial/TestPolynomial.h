#ifndef TESTPOLYNOMIAL_H
#define TESTPOLYNOMIAL_H

#include <cassert>
#include <cmath>
#include <iostream>
#include <map>
#include <ctime>
#include <vector>

template<class Polynomial>
class TestPolynomial {
public:
    bool testConstructor() const {
        std::cout << "Test Constructor" << std::endl;
        Polynomial p;
        p.coff(2) = 3;
        p.coff(0) = 5;

        const Polynomial p1(p);
        assert(nearlyEqual(p1.coff(2), 3.0));
        assert(nearlyEqual(p1.coff(0), 5.0));
        assert(nearlyEqual(p1.coff(7), 0.0));
        p1.Print();

        std::cout << std::endl;
        return true;
    }

    bool testOperationCorrectness() {
        std::cout << "Test OperationCorrectness" << std::endl;
        Polynomial p0;
        p0.coff(2) = 3;
        p0.coff(0) = 5;
        p0.coff(3) = 4;

        Polynomial p1;
        p1.coff(3) = -4;
        p1.coff(100) = 1;

        const Polynomial add = p0 + p1;
        const Polynomial sub = p0 - p1;
        const Polynomial mul = p0 * p1;

        assert(nearlyEqual(add.coff(0), 5.0));
        assert(nearlyEqual(add.coff(2), 3.0));
        assert(nearlyEqual(add.coff(3), 0.0));
        assert(nearlyEqual(add.coff(100), 1.0));

        assert(nearlyEqual(sub.coff(0), 5.0));
        assert(nearlyEqual(sub.coff(2), 3.0));
        assert(nearlyEqual(sub.coff(3), 8.0));
        assert(nearlyEqual(sub.coff(100), -1.0));

        assert(nearlyEqual(mul.coff(100), 5.0));
        assert(nearlyEqual(mul.coff(102), 3.0));
        assert(nearlyEqual(mul.coff(103), 4.0));
        assert(nearlyEqual(mul.coff(5), -12.0));
        assert(nearlyEqual(mul.coff(6), -16.0));

        std::cout << std::endl;
        return true;
    }

    bool testConstructorFromGivenData(const std::vector<int>& deg, const std::vector<double>& cof) {
        std::cout << "Test Constructor with Size: " << deg.size() << std::endl;
        clock_t t0 = clock();
        Polynomial p(deg, cof);
        assert(matchesPolynomial(p, buildReference(deg, cof)));
        std::cout << "Test Constructor time: " << clock() - t0 << std::endl;
        std::cout << std::endl;
        return true;
    }

    bool testOperationFromGivenData(const std::vector<int>& deg0, const std::vector<double>& cof0,
        const std::vector<int>& deg1, const std::vector<double>& cof1, bool showOutput = false)
    {
        clock_t t0 = clock();
        Polynomial p0(deg0, cof0);
        Polynomial p1(deg1, cof1);
        const Polynomial add = p0 + p1;
        const Polynomial sub = p0 - p1;
        const Polynomial mul = p0 * p1;
        const auto ref0 = buildReference(deg0, cof0);
        const auto ref1 = buildReference(deg1, cof1);
        assert(matchesPolynomial(add, addReference(ref0, ref1)));
        assert(matchesPolynomial(sub, subtractReference(ref0, ref1)));
        assert(matchesPolynomial(mul, multiplyReference(ref0, ref1)));

        if (showOutput) {
            mul.Print();
            add.Print();
            sub.Print();
        }

        std::cout << "Test Operation time: " << clock() - t0 << std::endl;
        std::cout << std::endl;
        return true;
    }

private:
    static bool nearlyEqual(double left, double right) {
        return std::abs(left - right) < 1e-8;
    }

    static std::map<int, double> buildReference(const std::vector<int>& deg, const std::vector<double>& cof) {
        std::map<int, double> terms;
        const std::size_t nSize = (deg.size() < cof.size()) ? deg.size() : cof.size();
        for (std::size_t i = 0; i < nSize; ++i) {
            if (deg[i] < 0) {
                continue;
            }
            terms[deg[i]] += cof[i];
        }
        compressReference(&terms);
        return terms;
    }

    static std::map<int, double> addReference(
        const std::map<int, double>& left,
        const std::map<int, double>& right)
    {
        std::map<int, double> result = left;
        for (const auto& [deg, cof] : right) {
            result[deg] += cof;
        }
        compressReference(&result);
        return result;
    }

    static std::map<int, double> subtractReference(
        const std::map<int, double>& left,
        const std::map<int, double>& right)
    {
        std::map<int, double> result = left;
        for (const auto& [deg, cof] : right) {
            result[deg] -= cof;
        }
        compressReference(&result);
        return result;
    }

    static std::map<int, double> multiplyReference(
        const std::map<int, double>& left,
        const std::map<int, double>& right)
    {
        std::map<int, double> result;
        for (const auto& [leftDeg, leftCof] : left) {
            for (const auto& [rightDeg, rightCof] : right) {
                result[leftDeg + rightDeg] += leftCof * rightCof;
            }
        }
        compressReference(&result);
        return result;
    }

    static void compressReference(std::map<int, double>* terms) {
        for (auto it = terms->begin(); it != terms->end();) {
            if (nearlyEqual(it->second, 0.0)) {
                it = terms->erase(it);
            }
            else {
                ++it;
            }
        }
    }

    static bool matchesPolynomial(const Polynomial& polynomial, const std::map<int, double>& expected) {
        for (const auto& [deg, cof] : expected) {
            if (!nearlyEqual(polynomial.coff(deg), cof)) {
                return false;
            }
        }

        for (int probe = 0; probe <= 110; ++probe) {
            const auto it = expected.find(probe);
            const double expectedCof = (it == expected.end()) ? 0.0 : it->second;
            if (!nearlyEqual(polynomial.coff(probe), expectedCof)) {
                return false;
            }
        }
        return nearlyEqual(polynomial.coff(-1), 0.0);
    }
};

#endif // TESTPOLYNOMIAL_H

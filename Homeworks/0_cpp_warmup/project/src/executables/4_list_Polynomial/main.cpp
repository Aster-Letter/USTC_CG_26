#include "PolynomialList.h"

#include <cassert>
#include <cmath>
#include <iostream>

namespace {
bool NearlyEqual(double left, double right) {
	return std::abs(left - right) < 1e-8;
}
}

int main(int argc, char** argv) {
	(void)argc;
	(void)argv;

	PolynomialList p1("../data/P3.txt");
	PolynomialList p2("../data/P4.txt");
	PolynomialList p3;
	p1.Print();
	p2.Print();
	assert(NearlyEqual(p1.coff(1), 2.0));
	assert(NearlyEqual(p1.coff(4), -3.0));
	assert(NearlyEqual(p2.coff(1), 4.0));
	assert(NearlyEqual(p2.coff(4), -3.0));

	p3 = p1 + p2;
	p3.Print();
	assert(NearlyEqual(p3.coff(1), 6.0));
	assert(NearlyEqual(p3.coff(4), -6.0));
	p3 = p1 - p2;
	p3.Print();
	assert(NearlyEqual(p3.coff(1), -2.0));
	assert(NearlyEqual(p3.coff(4), 0.0));

	p3 = p1 * p2;
	p3.Print();
	assert(NearlyEqual(p3.coff(2), 8.0));
	assert(NearlyEqual(p3.coff(5), -18.0));
	assert(NearlyEqual(p3.coff(8), 9.0));

	const double cof[] = {1.0, 2.0, -3.0};
	const int deg[] = {1, 1, 4};
	PolynomialList merged(cof, deg, 3);
	assert(NearlyEqual(merged.coff(1), 3.0));
	assert(NearlyEqual(merged.coff(4), -3.0));

	std::cout << "All PolynomialList checks passed." << std::endl;

	return 0;
}
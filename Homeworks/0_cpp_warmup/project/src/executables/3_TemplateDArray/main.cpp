#include "DArray.h"

#include <cassert>
#include <iostream>
#include <string>

int main(int argc, char** argv) {
	(void)argc;
	(void)argv;

	// Case 1: double basic workflow.
	DArray<double> a;
	a.InsertAt(0, 2.1);
	a.PushBack(3.0);
	a.PushBack(3.1);
	a.PushBack(3.2);
	a.DeleteAt(0);
	a.InsertAt(0, 4.1);
	assert(a.GetSize() == 4);
	assert(a.GetAt(0) == 4.1);
	a.Print();

	// Case 2: copy constructor / assignment and deep-copy behavior.
	DArray<double> acopy = a;
	DArray<double> acopy2(a);
	DArray<double> acopy3;
	DArray<double> acopy4;
	acopy4 = acopy3 = a;
	acopy.SetAt(0, 99.9);
	assert(a.GetAt(0) == 4.1);
	assert(acopy2.GetAt(0) == 4.1);
	assert(acopy3.GetAt(0) == 4.1);
	assert(acopy4.GetAt(0) == 4.1);

	// Case 3: int resize + operator[] read/write.
	DArray<int> b;
	b.PushBack(21);
	b.PushBack(22);
	b.SetSize(5);
	assert(b.GetSize() == 5);
	assert(b[0] == 21);
	assert(b[1] == 22);
	assert(b[2] == 0);
	b[2] = 100;
	assert(b.GetAt(2) == 100);
	b.Print();

	// Case 4: char insertion order.
	DArray<char> c;
	c.PushBack('a');
	c.PushBack('b');
	c.PushBack('c');
	c.InsertAt(0, 'd');
	assert(c.GetAt(0) == 'd');
	assert(c.GetAt(3) == 'c');
	c.Print();

	// Case 5: std::string template instance.
	DArray<std::string> s;
	s.PushBack("hello");
	s.PushBack("template");
	s.InsertAt(1, "dynamic");
	assert(s.GetSize() == 3);
	assert(s[0] == "hello");
	assert(s[1] == "dynamic");
	assert(s[2] == "template");
	s.Print();

	// Case 6: boundary checks should not crash.
	s.DeleteAt(99);
	s.SetAt(-1, "x");
	(void)s.GetAt(99);

	std::cout << "All template DArray checks passed." << std::endl;

    system("pause");
	return 0;
}

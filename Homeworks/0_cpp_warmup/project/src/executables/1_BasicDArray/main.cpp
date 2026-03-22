#include "DArray.h"

#include <cassert>
#include <iostream>

int main(int argc, char** argv) {
	(void)argc;
	(void)argv;

	DArray a;
	a.InsertAt(0, 2.1);
	a.PushBack(3.0);
	a.PushBack(3.1);
	a.PushBack(3.2);
	a.DeleteAt(0);
	a.InsertAt(0, 4.1);
	assert(a.GetSize() == 4);
	assert(a.GetAt(0) == 4.1);
	a.Print();

	DArray acopy = a;
	DArray acopy2(a);
	DArray acopy3;
	DArray acopy4;
	acopy4 = acopy3 = a;
	acopy.SetAt(0, 99.9);
	assert(a.GetAt(0) == 4.1);
	assert(acopy2.GetAt(0) == 4.1);
	assert(acopy3.GetAt(0) == 4.1);
	assert(acopy4.GetAt(0) == 4.1);

	DArray b;
	b.PushBack(21);
	b.DeleteAt(0);
	b.PushBack(22);
	b.SetSize(5);
	assert(b.GetSize() == 5);
	assert(b[0] == 22);
	assert(b[1] == 0.0);
	b.SetSize(2);
	assert(b.GetSize() == 2);
	b.Print();

	DArray c;
	c.PushBack('a');
	c.PushBack('b');
	c.PushBack('c');
	c.InsertAt(0, 'd');
	assert(c.GetAt(0) == 'd');
	assert(c.GetAt(3) == 'c');
	c.Print();

	a.DeleteAt(99);
	a.InsertAt(-1, 0.0);
	a.SetAt(99, 0.0);
	(void)a.GetAt(99);

	std::cout << "All basic DArray checks passed." << std::endl;
	return 0;
}
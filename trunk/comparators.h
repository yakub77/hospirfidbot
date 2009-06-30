#ifndef COMPARATORS_H
#define COMPARATORS_H

#include <map>
using namespace std;

struct CompareFloat {
	bool operator()(const float f1, const float f2) const {
		return (f1 < f2);
	}
};

struct CompareDouble {
	bool operator()(const double d1, const double d2) const {
		return (d1 < d2);
	}
};

struct CompareInt {
	bool operator()(const int i1, const int i2) const {
		return (i1 < i2);
	}
};

struct CompareString {
	bool operator()(const char* s1, const char* s2) const {
		return (strcmp(s1, s2) > 0);
	}
};

#endif

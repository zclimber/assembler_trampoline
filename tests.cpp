#include <iostream>
#include <fstream>
#include <vector>
#include <thread>
#include <iomanip>
#include <clocale>
#include <cstdlib>
#include <type_traits>
#include <algorithm>
#include <cstring>

#include <sys/mman.h>

#include "trampoline.h"

using std::cout;

struct em {
	int x;
	void operator ()(int a, int b, int c, int d, int e, int f, int g, int h) {
		x = a + b * 10 + c * 100 + d * 1000 + e * 10000 + f * 100000
				+ g * 1000000 + h * 10000000;
		asm("");
		cout << a << b << c << d << e << f << g << h << "\n";
	}
};

struct sec {
	void operator ()(int a, long double d, int c) {
		cout << a << " " << (int) d << " " << c << "\n";
	}
};

int main(int argc, char ** argv) {
	em * emm = new em();

	auto tp = make_trampoline(emm);

	auto fun = tp.to_func();

	fun(1, 2, 3, 4, 5, 6, 7, 8);
	fun(8, 7, 6, 5, 4, 3, 2, 1);

	sec * sc = new sec();

	auto tpp = make_trampoline(sc);
	auto ptr = tpp.to_func();

	ptr(1, 2.5, 3);

	ptr(10, 20.5, 30);

	auto lam = [](int a, int b, int c) {
		cout << a << b << c << "\n";
	};

	lam(1, 2, 3);

	auto tppp = make_trampoline(&lam);

	auto baz = tppp.to_func();

	baz(3, 4, 5);
	baz(5, 6, 7);

	auto tpppp = make_trampoline(
			[](double a, int b, double c, int d, double, int) {
				cout << b << d << "\n";
			});

	tpppp.to_func()(1.0, 2, 3.0, 4, 5.0, 6);

	cout << "DONE";

	return 0;
}

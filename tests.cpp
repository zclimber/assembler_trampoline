#include <iostream>

#include "trampoline.h"

using std::cout;

int main() {
	{
		struct em {
			int x;
			void operator ()(int a, int b, int c, int d, int e, int f, int g,
					int h) {
				x = a + b * 10 + c * 100 + d * 1000 + e * 10000 + f * 100000
						+ g * 1000000 + h * 10000000;
				asm("");
				cout << a << b << c << d << e << f << g << h << "\n";
			}
		};

		em emm;

		auto tp = make_trampoline(&emm);
		auto fun = tp.to_func();

		fun(1, 2, 3, 4, 5, 6, 7, 8);
		fun(8, 7, 6, 5, 4, 3, 2, 1);
	}
	{
		struct sec {
			void operator ()(int a, long double d, int c) {
				cout << a << " " << (int) d << " " << c << "\n";
			}
		};

		auto tp = make_trampoline(sec());
		auto fun = tp.to_func();

		fun(1, 2.5, 3);
		fun(10, 20.5, 30);
	}
	{
		auto lam = [](int a, int b, int c) {
			cout << a << b << c << "\n";
		};

		auto tp = make_trampoline(&lam);
		auto fun = tp.to_func();

		fun(3, 4, 5);
		fun(5, 6, 7);
	}
	{
		auto tp = make_trampoline(
				[](double a, int b, long double c, int d, double, int) {
					cout << (int) a << b << (int) c << d << "\n";
				});

		tp.to_func()(1.0, 2, 3.0, 4, 5.0, 6);
		tp.to_func()(6.0, 5, 4.0, 3, 2.0, 1);

	}
	for (int i = 0; i < 100000; i++) {
		auto tp = make_trampoline([]() {
			cout << "";
		});
		tp.to_func()();
	}

	cout << "DONE";
	return 0;
}

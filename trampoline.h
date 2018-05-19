/*
 * trampoline.h
 *
 *  Created on: Apr 28, 2018
 *      Author: mk2
 */

#ifndef TRAMPOLINE_H_
#define TRAMPOLINE_H_

#include <algorithm>
#include <memory>
#include <mutex>
#include <string>
#include <type_traits>
#include <vector>

class info {

	static std::mutex mut;
	static std::vector<void *> part_256, part_1024;

	static const int PAGE_SIZE = 4096;
	static const int PAGES_PER_REQUEST = 4;

	enum asked_mem {
		nomem, mem256, mem1024, fullmem
	} mem = nomem;

	void * given_mem = nullptr;
	unsigned int size = 0;

protected:
	info() = default;
	std::string to_str(int val);
	std::string to_str(const void * ptr);
public:
	enum types_of_arg {
		reg, sse_reg, stack
	};
	int integers = 0, sse = 0, memory = 0;
	std::vector<types_of_arg> args;
	std::string args_moving_code();
	std::string cleanup_code();

	void * get_mem(unsigned int size);
	~info();
};

template<class T, class Y>
struct supported_type;

template<>
struct supported_type<long double, void> {
	static void make_a(info & i) {
		i.memory += 2;
		i.args.push_back(i.stack);
		i.args.push_back(i.stack);
	}
};

template<class T>
struct supported_type<T, std::enable_if_t<std::is_integral<T>::value>> {
	static void make_a(info & i) {
		i.integers++;
		if (i.integers <= 6) {
			i.args.push_back(i.reg);
		} else {
			i.args.push_back(i.stack);
		}
	}
};

template<class T>
struct supported_type<T, std::enable_if_t<std::is_floating_point<T>::value>> {
	static void make_a(info & i) {
		i.sse++;
		if (i.sse <= 8) {
			i.args.push_back(i.sse_reg);
		} else {
			i.args.push_back(i.stack);
		}
	}
};

template<class ... Args>
struct maker;

template<>
struct maker<> {
	static void make(info &) {
	}
};

template<class I, class ... Tail>
struct maker<I, Tail...> {
	static void make(info & i) {
		supported_type<I, void>::make_a(i);
		maker<Tail...>::make(i);
	}
};

template<class ... Args>
class trampoline;

template<class T, class R, class ... Args>
class trampoline<T, R (T::*)(Args...)> : public info {
	typedef R (*functype)(Args...);
	typedef R (T::*member_func)(Args...);
	functype func;
	static R invoke(T * t, Args ... args) {
		return t->operator ()(std::forward<Args>(args)...);
	}
	functype init_func(const T * t) {
		maker<Args...>::make(*this);

		auto code = args_moving_code();
		code.append("\x48\xBF"); // movabs object, %rdi
		code.append(to_str(t));
		code.append("\x48\xB8"); // movabs function, %rax
		code.append(to_str(reinterpret_cast<void *>(&invoke)));
		code.append("\xFF\xD0"); // callq %rax
		code += cleanup_code();

		char * ptr = reinterpret_cast<char*>(get_mem(code.size()));
		std::copy(code.begin(), code.end(), ptr);
		return (functype) ptr;
	}
	std::unique_ptr<T> ptr;
public:
	trampoline(T * t) {
		func = init_func(t);
	}
	trampoline(const T & t) {
		func = init_func(&t);
	}
	trampoline(T && t) {
		ptr = std::make_unique<T>(std::move(t));
		func = init_func(ptr.get());
	}
	functype to_func() {
		return func;
	}
};

template<class T, class R, class ... Args>
class trampoline<T, R (T::*)(Args...) const> : public trampoline<T,
		R (T::*)(Args...)> {
public:
	trampoline(T * t) :
			trampoline<T, R (T::*)(Args...)>(t) {
	}
	trampoline(const T & t) :
			trampoline<T, R (T::*)(Args...)>(t) {
	}
	trampoline(T && t) :
			trampoline<T, R (T::*)(Args...)>(t) {
	}
};

template<class T>
auto make_trampoline(T * t) {
	typedef trampoline<T, decltype(&T::operator())> tramp;
	return tramp(t);
}

template<class T>
auto make_trampoline(const T & t) {
	typedef trampoline<T, decltype(&T::operator())> tramp;
	return tramp(t);
}

template<class T>
auto make_trampoline(T && t) {
	typedef trampoline<T, decltype(&T::operator())> tramp;
	return tramp(std::forward<T>(t));
}

#endif /* TRAMPOLINE_H_ */

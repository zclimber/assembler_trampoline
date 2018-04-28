/*
 * trampoline.cpp
 *
 *  Created on: Apr 28, 2018
 *      Author: mk2
 */

#include "trampoline.h"
#include <sys/mman.h>

std::mutex info::mut;
std::vector<void *> info::part_256, info::part_1024;

void * info::get_mem(unsigned int size) {
	if (size > 1024) {
		given_mem = mmap(nullptr, size, PROT_READ | PROT_WRITE | PROT_EXEC,
		MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
	} else {
		std::lock_guard<std::mutex> lg(mut);
		int divisor;
		std::vector<void *> & ref = part_256;
		if (size <= 256) {
			ref = part_256;
			divisor = 256;
		} else {
			ref = part_1024;
			divisor = 1024;
		}
		if (ref.empty()) {
			void * ptr = mmap(nullptr, PAGE_SIZE * PAGES_PER_REQUEST,
			PROT_READ | PROT_WRITE | PROT_EXEC,
			MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
			long long v = reinterpret_cast<long long>(ptr);
			for (long long vv = v; vv < v + PAGE_SIZE * PAGES_PER_REQUEST; vv +=
					divisor) {
				ref.push_back(reinterpret_cast<void *>(vv));
			}
		}
		given_mem = ref.back();
		ref.pop_back();
	}
	return given_mem;
}

info::~info() {
	if (mem == mem256 || mem == mem1024) {
		std::lock_guard<std::mutex> lg(mut);
		if (mem == mem256) {
			part_256.push_back(given_mem);
		} else {
			part_1024.push_back(given_mem);
		}
	} else if (mem == fullmem) {
		munmap(given_mem, size);
	}
}

std::string info::to_str(int val) {
	std::string str(4, 0);
	for (int i = 0; i < 4; i++) {
		str[i] = val;
		val >>= 8;
	}
	return str;
}

std::string info::to_str(const void * ptr) {
	long long ll = reinterpret_cast<long long>(ptr);
	std::string str(8, 0);
	for (int i = 0; i < 8; i++) {
		str[i] = ll;
		ll >>= 8;
	}
	return str;
}

std::string info::args_moving_code() {
	std::string code;
	std::vector<std::string> reg_move = { "\x48\x89\xfe", // mov    %rdi,%rsi
			"\x48\x89\xf2", // mov    %rsi,%rdx
			"\x48\x89\xd1", // mov    %rdx,%rcx
			"\x49\x89\xc8", // mov    %rcx,%r8d
			"\x4d\x89\xc1", // mov    %r8d,%r9d
			"\x41\x51" 		// push   %r9
			};
	reg_move.resize(std::min(integers, 6));

	int stacksize = (std::max(0, integers - 6) + std::max(0, sse - 8) + memory)
			* 8;
	std::reverse(args.begin(), args.end());
	for (auto arg : args) {
		switch (arg) {
		case reg:
			code.append(reg_move.back());
			reg_move.pop_back();
			break;
		case stack:
			if (stacksize <= INT8_MAX) {
				code.append("\x48\x8B\x44\x24"); // mov    stacksize(%rsp),%rax
				code.push_back(stacksize);
			} else {
				code.append("\x48\x8B\x84\x24"); // mov    stacksize(%rsp),%rax
				code.append(to_str(stacksize));
			}
			code.append("\x50"); // push %rax
			break;
		case sse_reg:
			break;
		}
	}
	return code;
}

std::string info::cleanup_code() {
	std::string code;
	int stacksize = (std::max(0, integers - 6) + std::max(0, sse - 8) + memory)
			* 8;
	if (integers >= 6)
		stacksize += 8;
	if (stacksize <= INT8_MAX) {
		code.append("\x48\x83\xC4"); // add    stacksize + 8,%rsp
		code.push_back(stacksize);
	} else {
		code.append("\x48\x81\xC4"); // add    stacksize + 8,%rsp
		code.append(to_str(stacksize));
	}
	code.append("\xC3");
	return code;
}

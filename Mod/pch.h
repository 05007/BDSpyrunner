#pragma once
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <iostream>
#include <fstream>
#include <string>
#include <thread>
#include <vector>
#include <unordered_map>
#include "include/Python.h"
using VA = unsigned long long;
extern "C" {
	_declspec(dllimport) int HookFunction(void*, void*, void*);
	_declspec(dllimport) void* GetServerSymbol(const char*);
}
template<typename ret, typename... Args>
inline ret SYMCALL(const char* fn, Args... args) {
	return ((ret(*)(Args...))GetServerSymbol(fn))(args...);
}
void* HookRegister(char const* sym, void* hook, void* org) {
	void* found = GetServerSymbol(sym);
	if (!found) puts(sym);
	HookFunction(found, &org, hook); return org;
}
#define Hook(name, ret, sym, ...)		\
struct name {							\
	static ret(*original)(__VA_ARGS__);	\
	static ret _hook(__VA_ARGS__);		\
};										\
ret(*name::original)(__VA_ARGS__)=(ret(*)(__VA_ARGS__))HookRegister(sym, &name::_hook, &name::original);\
ret name::_hook(__VA_ARGS__)
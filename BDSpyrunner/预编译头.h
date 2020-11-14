#pragma once
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <iostream>
#include <string>
using VA = unsigned long long;
extern "C" {
	_declspec(dllimport) int HookFunction(void* oldfunc, void** poutold, void* newfunc);
	_declspec(dllimport) void* GetServerSymbol(char const* name);
}
template<typename ret, typename... Args>
static inline ret SYMCALL(const char* fn, Args... args) { return ((ret(*)(Args...))GetServerSymbol(fn))(args...); }
struct HookRegister {
	HookRegister(char const* sym, void* hook, void** org) {
		void* found = GetServerSymbol(sym);
		if (found) {HookFunction(found, org, hook);return;}
		printf("FailedToHook: %s\n", sym); exit(-1);
	}
};
#define original(...) _original()(__VA_ARGS__)
#define THook(name,ret,sym,...)				\
struct name {									\
	typedef ret(*func)(__VA_ARGS__);			\
	static func& _original() {static func storage = nullptr;return storage;}\
	static ret _hook(__VA_ARGS__);				\
};												\
HookRegister name{sym,&name::_hook,(void**)&name::_original()};\
ret name::_hook(__VA_ARGS__)

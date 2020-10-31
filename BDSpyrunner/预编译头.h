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
inline ret SYMCALL(const char* fn, Args... args) {
	return ((ret(*)(Args...))GetServerSymbol(fn))(args...);
}
struct HookRegister {
	HookRegister(void* address, void* hook, void** org) {
		if (HookFunction(address, org, hook) != 0) printf("FailedToHook: %p\n", address);
	}
	HookRegister(char const* sym, void* hook, void** org) {
		auto found = GetServerSymbol(sym);
		if (found == nullptr) printf("FailedToHook: %p\n", sym);
		if (HookFunction(found, org, hook)!= 0) printf("FailedToHook: %s\n", sym);
	}
};
#define SYMHOOK(name,ret,sym,...)				\
struct name {									\
	typedef ret(*original_type)(__VA_ARGS__);	\
	static original_type& _original() {			\
		static original_type storage;			\
		return storage;							\
	}											\
	template <typename... T>					\
	static ret original(T&&... params) {		\
		return _original()(std::forward<T>(params)...);\
	}											\
	static ret _hook(__VA_ARGS__);				\
};												\
HookRegister name{sym,&name::_hook,(void**)&name::_original()};\
ret name::_hook(__VA_ARGS__)

#pragma once
using VA = unsigned long long;
extern "C" {
	_declspec(dllimport) char HookFunction(void* oldfunc, void** poutold, void* newfunc);
	_declspec(dllimport) void* GetServerSymbol(const char* name);
}
template<typename ret, typename... Args>
inline ret SYMCALL(const char* fn, Args... args) {
	return ((ret(*)(Args...))GetServerSymbol(fn))(args...);
}
struct HookRegister {
	HookRegister(char const* sym, void* hook, void** org) {
		void* found = GetServerSymbol(sym);
		if (found) { HookFunction(found, org, hook); return; }
		printf("FailedToHook: %s\n", sym); exit(-1);
	}
};
#define original _original()
#define THook(name,ret,sym,...)				\
struct name {									\
	static auto& _original() {static ret(*func)(__VA_ARGS__) = nullptr;return func;}\
	static ret _hook(__VA_ARGS__);				\
};												\
HookRegister name{sym,&name::_hook,(void**)&name::_original()};\
ret name::_hook(__VA_ARGS__)

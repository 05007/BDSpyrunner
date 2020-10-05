#ifndef PCH_H
#define PCH_H

//----------------------------------
// ����ͷ�ļ�
//----------------------------------
#define WIN32_LEAN_AND_MEAN
// Windows ͷ�ļ�
#include <windows.h>
// C++��׼�� ͷ�ļ�
#include <iostream>
#include <string>
#include <unordered_map>
#include <map>
#include <string>
#include <string_view>
#define PY_SSIZE_T_CLEAN
#include "include/Python.h"
extern "C" {
	_declspec(dllimport) int HookFunction(void* oldfunc, void** poutold, void* newfunc);
	_declspec(dllimport) void* GetServerSymbol(char const* name);
}
//----------------------------------
// �������Ͷ���
//----------------------------------
using VA = unsigned __int64;
using RVA = unsigned int;
template<typename Type>
using Ptr = Type*;
typedef unsigned long long CHash;
constexpr CHash do_hash(std::string_view x) {
	CHash rval = 0;
	for (size_t i = 0; i < x.size(); ++i) {
		rval *= 131;
		rval += x[i];
		rval += 0;
	}
	return rval;
}
constexpr CHash do_hash2(std::string_view x) {
	//ap hash
	CHash rval = 0;
	for (size_t i = 0; i < x.size(); ++i) {
		if (i & 1) {
			rval ^= (~((rval << 11) ^ x[i] ^ (rval >> 5)));
		}
		else {
			rval ^= (~((rval << 7) ^ x[i] ^ (rval >> 3)));
		}
	}
	return rval;
}
template <typename T, int off>
inline T& dAccess(void* ptr) {
	return *(T*)(((uintptr_t)ptr) + off);
}
template <typename T, int off>
inline T const& dAccess(void const* ptr) {
	return *(T*)(((uintptr_t)ptr) + off);
}
template <typename T>
inline T& dAccess(void* ptr, uintptr_t off) {
	return *(T*)(((uintptr_t)ptr) + off);
}
template <typename T>
inline const T& dAccess(void const* ptr, uintptr_t off) {
	return *(T*)(((uintptr_t)ptr) + off);
}
#define __WEAK __declspec(selectany)

#define SYM(x) GetServerSymbol(x)

template<typename T_RET, typename... Args>
T_RET Symcall(const char* fn, Args... args) {
	using FnType = T_RET(*)(Args...);
	FnType p = (FnType)SYM(fn);
	return p(args...);
}
#define SYMCALL(ret, sym, ...) (Symcall<ret>(sym, ##__VA_ARGS__))
//#define SymCall(ret,fn, ...) ((ret(*)(__VA_ARGS__))(SYM(fn)))
class THookRegister {
public:
	THookRegister(void* address, void* hook, void** org) {
		auto ret = HookFunction(address, org, hook);
		if (ret != 0) {
			printf("FailedToHook: %p\n", address);
		}
	}
	THookRegister(char const* sym, void* hook, void** org) {
		auto found = GetServerSymbol(sym);
		if (found == nullptr) {
			printf("FailedToHook: %p\n", sym);
		}
		auto ret = HookFunction(found, org, hook);
		if (ret != 0) {
			printf("FailedToHook: %s\n", sym);
		}
	}
	template <typename T>
	THookRegister(const char* sym, T hook, void** org) {
		union {
			T a;
			void* b;
		} hookUnion;
		hookUnion.a = hook;
		THookRegister(sym, hookUnion.b, org);
	}
	template <typename T>
	THookRegister(void* sym, T hook, void** org) {
		union {
			T a;
			void* b;
		} hookUnion;
		hookUnion.a = hook;
		THookRegister(sym, hookUnion.b, org);
	}
};
template <CHash, CHash>
struct THookTemplate;
template <CHash, CHash>
extern THookRegister THookRegisterTemplate;

#define _TInstanceHook(class_inh, pclass, iname, sym, ret, ...)                                             \
	template <>                                                                                             \
	struct THookTemplate<do_hash(iname), do_hash2(iname)> class_inh {                                                        \
		typedef ret (THookTemplate::*original_type)(__VA_ARGS__);                                           \
		static original_type& _original() {                                                                 \
			static original_type storage;                                                                   \
			return storage;                                                                                 \
		}                                                                                                   \
		template <typename... Params>                                                                       \
		static ret original(pclass* _this, Params&&... params) {                                            \
			return (((THookTemplate*)_this)->*_original())(std::forward<Params>(params)...);                \
		}                                                                                                   \
		ret _hook(__VA_ARGS__);                                                                             \
	};                                                                                                      \
	template <>                                                                                             \
	static THookRegister THookRegisterTemplate<do_hash(iname), do_hash2(iname)>{ sym, &THookTemplate<do_hash(iname), do_hash2(iname)>::_hook, \
		(void**)&THookTemplate<do_hash(iname), do_hash2(iname)>::_original() };                                              \
	ret THookTemplate<do_hash(iname), do_hash2(iname)>::_hook(__VA_ARGS__)

#define _TInstanceDefHook(iname, sym, ret, type, ...) \
	_TInstanceHook(                                   \
		: public type, type, iname, sym, ret, __VA_ARGS__)
#define _TInstanceNoDefHook(iname, sym, ret, ...) _TInstanceHook(, void, iname, sym, ret, __VA_ARGS__)

#define _TStaticHook(pclass, iname, sym, ret, ...)                                                          \
	template <>                                                                                             \
	struct THookTemplate<do_hash(iname), do_hash2(iname)> pclass {                                                           \
		typedef ret (*original_type)(__VA_ARGS__);                                                          \
		static original_type& _original() {                                                                 \
			static original_type storage;                                                                   \
			return storage;                                                                                 \
		}                                                                                                   \
		template <typename... Params>                                                                       \
		static ret original(Params&&... params) {                                                          \
			return _original()(std::forward<Params>(params)...);                                            \
		}                                                                                                   \
		static ret _hook(__VA_ARGS__);                                                                      \
	};                                                                                                      \
	template <>                                                                                             \
	static THookRegister THookRegisterTemplate<do_hash(iname), do_hash2(iname)>{ sym, &THookTemplate<do_hash(iname), do_hash2(iname)>::_hook, \
		(void**)&THookTemplate<do_hash(iname), do_hash2(iname)>::_original() };                                              \
	ret THookTemplate<do_hash(iname), do_hash2(iname)>::_hook(__VA_ARGS__)

#define _TStaticDefHook(iname, sym, ret, type, ...) \
	_TStaticHook(                                   \
		: public type, iname, sym, ret, __VA_ARGS__)
#define _TStaticNoDefHook(iname, sym, ret, ...) _TStaticHook(, iname, sym, ret, __VA_ARGS__)

#define THook2(iname, ret, sym, ...) _TStaticNoDefHook(iname, sym, ret, __VA_ARGS__)
#define THook(ret, sym, ...) THook2(sym, ret, sym, __VA_ARGS__)
#define TStaticHook2(iname, ret, sym, type, ...) _TStaticDefHook(iname, sym, ret, type, __VA_ARGS__)
#define TStaticHook(ret, sym, type, ...) TStaticHook2(sym, ret, sym, type, __VA_ARGS__)
#define TClasslessInstanceHook2(iname, ret, sym, ...) _TInstanceNoDefHook(iname, sym, ret, __VA_ARGS__)
#define TClasslessInstanceHook(ret, sym, ...) TClasslessInstanceHook2(sym, ret, sym, __VA_ARGS__)
#define TInstanceHook2(iname, ret, sym, type, ...) _TInstanceDefHook(iname, sym, ret, type, __VA_ARGS__)
#define TInstanceHook(ret, sym, type, ...) TInstanceHook2(sym, ret, sym, type, __VA_ARGS__)
#endif //PCH_H
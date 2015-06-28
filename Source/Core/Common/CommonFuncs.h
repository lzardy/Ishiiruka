// Copyright 2009 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#pragma once

#ifdef _WIN32
#define SLEEP(x) Sleep(x)
#else
#include <unistd.h>
#define SLEEP(x) usleep(x*1000)
#endif

#ifdef __APPLE__
#include <libkern/OSByteOrder.h>
#endif

#include <clocale>
#include <cstddef>
#include <string>
#include <type_traits>
#include "Common/CommonTypes.h"

// Will fail to compile on a non-array:
// TODO: make this a function when constexpr is available
template <typename T>
struct ArraySizeImpl : public std::extent<T>
{ static_assert(std::is_array<T>::value, "is array"); };

#define ArraySize(x) ArraySizeImpl<decltype(x)>::value

#define b2(x)   (   (x) | (   (x) >> 1) )
#define b4(x)   ( b2(x) | ( b2(x) >> 2) )
#define b8(x)   ( b4(x) | ( b4(x) >> 4) )
#define b16(x)  ( b8(x) | ( b8(x) >> 8) )
#define b32(x)  (b16(x) | (b16(x) >>16) )
#define ROUND_UP_POW2(x)  (b32(x - 1) + 1)

#ifndef _WIN32

#include <errno.h>
#ifdef __linux__
#include <byteswap.h>
#elif defined __FreeBSD__
#include <sys/endian.h>
#endif

// go to debugger mode
	#ifdef _M_X86
		#define Crash() {asm ("int $3");}
	#else
		#define Crash() { exit(1); }
	#endif

// GCC 4.8 defines all the rotate functions now
// Small issue with GCC's lrotl/lrotr intrinsics is they are still 32bit while we require 64bit
#ifndef _rotl
inline u32 _rotl(u32 x, int shift)
{
	shift &= 31;
	if (!shift) return x;
	return (x << shift) | (x >> (32 - shift));
}

inline u32 _rotr(u32 x, int shift)
{
	shift &= 31;
	if (!shift) return x;
	return (x >> shift) | (x << (32 - shift));
}
#endif

inline u64 _rotl64(u64 x, unsigned int shift)
{
	unsigned int n = shift % 64;
	return (x << n) | (x >> (64 - n));
}

inline u64 _rotr64(u64 x, unsigned int shift)
{
	unsigned int n = shift % 64;
	return (x >> n) | (x << (64 - n));
}

#else // WIN32
// Function Cross-Compatibility
	#define strcasecmp _stricmp
	#define strncasecmp _strnicmp
	#define unlink _unlink
	#define snprintf _snprintf
	#define vscprintf _vscprintf

// Locale Cross-Compatibility
struct d_locale_t : localeinfo_struct
{
	u32 Mask;
	const char* Locale;
	d_locale_t(u32 mask, const char* locale)
	{
		Mask = mask;
		Locale = locale;
		m_innerlocale = _create_locale(mask, locale);
		if (m_innerlocale)
		{
			locinfo = m_innerlocale->locinfo;
			mbcinfo = m_innerlocale->mbcinfo;
		}
		else
		{
			locinfo = NULL;
			mbcinfo = NULL;
		}
	}
	~d_locale_t()
	{
		if (m_innerlocale)
		{
			_free_locale(m_innerlocale);
		}
	}
private:
	_locale_t m_innerlocale;
};
typedef d_locale_t* locale_t;

#define newlocale(mask, locale, base) new d_locale_t(mask, locale)
	#define LC_GLOBAL_LOCALE    ((locale_t)-1)
	#define LC_ALL_MASK         LC_ALL
	#define LC_COLLATE_MASK     LC_COLLATE
	#define LC_CTYPE_MASK       LC_CTYPE
	#define LC_MONETARY_MASK    LC_MONETARY
	#define LC_NUMERIC_MASK     LC_NUMERIC
	#define LC_TIME_MASK        LC_TIME
inline void  freelocale(locale_t l)
{
	if (l != LC_GLOBAL_LOCALE && l != NULL)
	{
		delete l;
	}
}

	inline locale_t uselocale(locale_t new_locale)
	{
		// Retrieve the current per thread locale setting
		bool bIsPerThread = (_configthreadlocale(0) == _ENABLE_PER_THREAD_LOCALE);

		// Retrieve the current thread-specific locale
	locale_t old_locale = NULL;
	if (new_locale != LC_GLOBAL_LOCALE && new_locale != NULL)
		old_locale = bIsPerThread ? new d_locale_t(new_locale->Mask, setlocale(new_locale->Mask, NULL)) : LC_GLOBAL_LOCALE;

		if (new_locale == LC_GLOBAL_LOCALE)
		{
			// Restore the global locale
			_configthreadlocale(_DISABLE_PER_THREAD_LOCALE);
		}
		else if (new_locale != nullptr)
		{
			// Configure the thread to set the locale only for this thread
			_configthreadlocale(_ENABLE_PER_THREAD_LOCALE);
			setlocale(new_locale->Mask, new_locale->Locale);
		}

		return old_locale;
	}

// 64 bit offsets for windows
	#define fseeko _fseeki64
	#define ftello _ftelli64
	#define atoll _atoi64
	#define stat64 _stat64
	#define fstat64 _fstat64
	#define fileno _fileno

extern "C"
{
	__declspec(dllimport) void __stdcall DebugBreak(void);
}
	#define Crash() {DebugBreak();}

	#if (_MSC_VER > 1800)
	#error alignof compat can be removed
	#else
	#define alignof(x) __alignof(x)
	#endif
#endif // WIN32 ndef

// Generic function to get last error message.
// Call directly after the command or use the error num.
// This function might change the error code.
// Defined in Misc.cpp.
std::string GetLastErrorMsg();

namespace Common
{
inline u8 swap8(u8 _data) {return _data;}
inline u32 swap24(const u8* _data) {return (_data[0] << 16) | (_data[1] << 8) | _data[2];}

#ifdef ANDROID
#undef swap16
#undef swap32
#undef swap64
#endif

#ifdef _WIN32
inline u16 swap16(u16 _data) {return _byteswap_ushort(_data);}
inline u32 swap32(u32 _data) {return _byteswap_ulong (_data);}
inline u64 swap64(u64 _data) {return _byteswap_uint64(_data);}
#elif __linux__ && !(ANDROID && _M_ARM_64)
// Android NDK r10c has broken builtin byte swap routines
// Disabled for now.
inline u16 swap16(u16 _data) {return bswap_16(_data);}
inline u32 swap32(u32 _data) {return bswap_32(_data);}
inline u64 swap64(u64 _data) {return bswap_64(_data);}
#elif __APPLE__
inline __attribute__((always_inline)) u16 swap16(u16 _data)
	{return OSSwapInt16(_data);}
inline __attribute__((always_inline)) u32 swap32(u32 _data)
	{return OSSwapInt32(_data);}
inline __attribute__((always_inline)) u64 swap64(u64 _data)
	{return OSSwapInt64(_data);}
#elif __FreeBSD__
inline u16 swap16(u16 _data) {return bswap16(_data);}
inline u32 swap32(u32 _data) {return bswap32(_data);}
inline u64 swap64(u64 _data) {return bswap64(_data);}
#else
// Slow generic implementation.
inline u16 swap16(u16 data) {return (data >> 8) | (data << 8);}
inline u32 swap32(u32 data) {return (swap16(data) << 16) | swap16(data >> 16);}
inline u64 swap64(u64 data) {return ((u64)swap32(data) << 32) | swap32(data >> 32);}
#endif

inline u16 swap16(const u8* _pData) {return swap16(*(const u16*)_pData);}
inline u32 swap32(const u8* _pData) {return swap32(*(const u32*)_pData);}
inline u64 swap64(const u8* _pData) {return swap64(*(const u64*)_pData);}

template <int count>
void swap(u8*);

template <>
inline void swap<1>(u8* data)
{}

template <>
inline void swap<2>(u8* data)
{
	*reinterpret_cast<u16*>(data) = swap16(data);
}

template <>
inline void swap<4>(u8* data)
{
	*reinterpret_cast<u32*>(data) = swap32(data);
}

template <>
inline void swap<8>(u8* data)
{
	*reinterpret_cast<u64*>(data) = swap64(data);
}

template <typename T>
inline T FromBigEndian(T data)
{
	//static_assert(std::is_arithmetic<T>::value, "function only makes sense with arithmetic types");

	swap<sizeof(data)>(reinterpret_cast<u8*>(&data));
	return data;
}

}  // Namespace Common

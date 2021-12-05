#pragma once

#if _WIN32
#include <cstdlib>

# define BYTE_SWAP_2(x) _byteswap_ushort(x)
# define BYTE_SWAP_4(x) _byteswap_ulong(x)
# define BYTE_SWAP_8(x) _byteswap_uint64(x)

#elif __linux__
#include <endian.h>
#include <byteswap.h>

#if !defined(BYTE_ORDER) && !defined(LITTLE_ENDIAN) && !defined(BIG_ENDIAN)
#define BYTE_ORDER		__BYTE_ORDER__
#define LITTLE_ENDIAN	__ORDER_LITTLE_ENDIAN__
#define BIG_ENDIAN		__ORDER_BIG_ENDIAN__
#endif

# define BYTE_SWAP_2(x) bswap_16(x)
# define BYTE_SWAP_4(x) bswap_32(x)
# define BYTE_SWAP_8(x) bswap_64(x)

#elif __APPLE__
#include <machine/endian.h>

# define BYTE_SWAP_2(x) __builtin_bswap16(x)
# define BYTE_SWAP_4(x) __builtin_bswap32(x)
# define BYTE_SWAP_8(x) __builtin_bswap64(x)

#endif

template<typename T, std::size_t size> struct type_size { typedef T type; };
template<typename T> struct type_size<T, sizeof(uint16_t)> { typedef uint16_t type; };
template<typename T> struct type_size<T, sizeof(uint32_t)> { typedef uint32_t type; };
template<typename T> struct type_size<T, sizeof(uint64_t)> { typedef uint64_t type; };

template<typename T> inline void hton_impl(T &v) { }

inline void hton_impl(uint16_t &v) { v = BYTE_SWAP_2(v); }

inline void hton_impl(uint32_t &v) { v = BYTE_SWAP_4(v); }

inline void hton_impl(uint64_t &v) { v = BYTE_SWAP_8(v); }

template<typename T, typename std::enable_if<std::is_pod<T>::value, int>::type = 0>
constexpr void hton(T &val)
{
#if _WIN32 || (defined(BYTE_ORDER) && defined(LITTLE_ENDIAN) && (BYTE_ORDER == LITTLE_ENDIAN))
	typedef typename type_size<T, sizeof(T)>::type i_type;
	hton_impl(reinterpret_cast<i_type &>(val));
#endif
}

template<typename T>
constexpr void ntoh(T &val)
{
	hton(val);
}

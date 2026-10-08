/* bswap16/32/64 of <byteswap.h> and <endian.h>: ixemul 80's syscall table
 * names them (626-628), but no library source implements them and the
 * SDK's libc.a has no stubs, so findutils 4.11 (locate's word_io.c) failed
 * to link on bswap32. Plain C: gcc turns these into rol/swap sequences. */
#include <sys/types.h>
#include <byteswap.h>

u_int16_t
bswap16(u_int16_t x)
{
	return (u_int16_t)(x << 8 | x >> 8);
}

u_int32_t
bswap32(u_int32_t x)
{
	return (x << 24) | ((x & 0xff00) << 8) | ((x >> 8) & 0xff00) | (x >> 24);
}

u_int64_t
bswap64(u_int64_t x)
{
	return (u_int64_t)bswap32((u_int32_t)x) << 32 | bswap32((u_int32_t)(x >> 32));
}

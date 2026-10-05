#ifndef _BYTESWAP_H_
#define _BYTESWAP_H_

#include <sys/types.h>

/* Assembly implementations */
u_int16_t bswap16(u_int16_t);
u_int32_t bswap32(u_int32_t);
u_int64_t bswap64(u_int64_t);

/* BSD/glibc-style aliases */
#define bswap_16(x) bswap16((u_int16_t)(x))
#define bswap_32(x) bswap32((u_int32_t)(x))
#define bswap_64(x) bswap64((u_int64_t)(x))

/* Internal-style aliases */
#define __bswap16(x)  bswap16((u_int16_t)(x))
#define __bswap32(x)  bswap32((u_int32_t)(x))
#define __bswap64(x)  bswap64((u_int64_t)(x))

#define __bswap_16(x) bswap16((u_int16_t)(x))
#define __bswap_32(x) bswap32((u_int32_t)(x))
#define __bswap_64(x) bswap64((u_int64_t)(x))

#endif /* _BYTESWAP_H_ */
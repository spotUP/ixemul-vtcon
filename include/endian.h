#ifndef _ENDIAN_H_
#define _ENDIAN_H_

#include <sys/types.h>
#include <byteswap.h>

/* Basic byte order constants. m68k is big endian. */
#define __LITTLE_ENDIAN 1234
#define __BIG_ENDIAN    4321
#define __BYTE_ORDER    __BIG_ENDIAN

#define LITTLE_ENDIAN __LITTLE_ENDIAN
#define BIG_ENDIAN    __BIG_ENDIAN
#define BYTE_ORDER    __BYTE_ORDER

/* Host <-> endian conversions (big-endian host) */
#define htobe16(x) ((u_int16_t)(x))
#define be16toh(x) ((u_int16_t)(x))
#define htole16(x) bswap16((u_int16_t)(x))
#define le16toh(x) bswap16((u_int16_t)(x))

#define htobe32(x) ((u_int32_t)(x))
#define be32toh(x) ((u_int32_t)(x))
#define htole32(x) bswap32((u_int32_t)(x))
#define le32toh(x) bswap32((u_int32_t)(x))

#define htobe64(x) ((u_int64_t)(x))
#define be64toh(x) ((u_int64_t)(x))
#define htole64(x) bswap64((u_int64_t)(x))
#define le64toh(x) bswap64((u_int64_t)(x))

#define betoh16(x) be16toh(x)
#define letoh16(x) le16toh(x)
#define betoh32(x) be32toh(x)
#define letoh32(x) le32toh(x)
#define betoh64(x) be64toh(x)
#define letoh64(x) le64toh(x)

#endif /* _ENDIAN_H_ */

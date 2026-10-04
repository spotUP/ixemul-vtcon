/* langinfo.h for the ixemul SDK (UP-Term): nl_langinfo from libixcompat.a,
 * with POSIX's full item set (gnulib's nl_langinfo wrapper needs MON_1,
 * ABDAY_1, ...). ixemul's own locale is "C" on AmigaOS's character set;
 * libixcompat adds UTF-8 for LC_CTYPE (setlocale, <wchar.h>), so CODESET is
 * "UTF-8" once setlocale selected a UTF-8 locale, else "ISO8859-1". The
 * strings are the C locale's (English). Without this header the toolchain's
 * newlib <langinfo.h> was found, and its <sys/features.h> defined
 * _POSIX_SOURCE, which hides half of ixemul's <stdlib.h>. */
#ifndef _LANGINFO_H_
#define _LANGINFO_H_

#include <sys/cdefs.h>

typedef int nl_item;

#define	CODESET		0	/* the character set */
#define	D_T_FMT		1	/* date and time format */
#define	D_FMT		2	/* date format */
#define	T_FMT		3	/* time format */
#define	RADIXCHAR	4	/* decimal point */
#define	THOUSEP		5	/* thousands separator */
#define	YESEXPR		6	/* affirmative answer, a regular expression */
#define	NOEXPR		7	/* negative answer */
#define	T_FMT_AMPM	8	/* 12-hour time format */
#define	AM_STR		9
#define	PM_STR		10
#define	ERA		11
#define	ERA_D_FMT	12
#define	ERA_D_T_FMT	13
#define	ERA_T_FMT	14
#define	ALT_DIGITS	15
#define	CRNCYSTR	16	/* currency symbol, with its position */
#define	DAY_1		17	/* Sunday ... DAY_7 Saturday */
#define	DAY_2		18
#define	DAY_3		19
#define	DAY_4		20
#define	DAY_5		21
#define	DAY_6		22
#define	DAY_7		23
#define	ABDAY_1		24	/* Sun ... ABDAY_7 Sat */
#define	ABDAY_2		25
#define	ABDAY_3		26
#define	ABDAY_4		27
#define	ABDAY_5		28
#define	ABDAY_6		29
#define	ABDAY_7		30
#define	MON_1		31	/* January ... MON_12 December */
#define	MON_2		32
#define	MON_3		33
#define	MON_4		34
#define	MON_5		35
#define	MON_6		36
#define	MON_7		37
#define	MON_8		38
#define	MON_9		39
#define	MON_10		40
#define	MON_11		41
#define	MON_12		42
#define	ABMON_1		43	/* Jan ... ABMON_12 Dec */
#define	ABMON_2		44
#define	ABMON_3		45
#define	ABMON_4		46
#define	ABMON_5		47
#define	ABMON_6		48
#define	ABMON_7		49
#define	ABMON_8		50
#define	ABMON_9		51
#define	ABMON_10	52
#define	ABMON_11	53
#define	ABMON_12	54
/* POSIX.1-2024: month names as they stand alone (the same in the C locale) */
#define	ALTMON_1	55
#define	ALTMON_2	56
#define	ALTMON_3	57
#define	ALTMON_4	58
#define	ALTMON_5	59
#define	ALTMON_6	60
#define	ALTMON_7	61
#define	ALTMON_8	62
#define	ALTMON_9	63
#define	ALTMON_10	64
#define	ALTMON_11	65
#define	ALTMON_12	66
#define	ABALTMON_1	67
#define	ABALTMON_2	68
#define	ABALTMON_3	69
#define	ABALTMON_4	70
#define	ABALTMON_5	71
#define	ABALTMON_6	72
#define	ABALTMON_7	73
#define	ABALTMON_8	74
#define	ABALTMON_9	75
#define	ABALTMON_10	76
#define	ABALTMON_11	77
#define	ABALTMON_12	78

__BEGIN_DECLS
char *nl_langinfo __P((nl_item));
__END_DECLS

#endif /* _LANGINFO_H_ */

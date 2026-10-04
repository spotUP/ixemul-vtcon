/* nl_langinfo for ixemul's C locale (see <langinfo.h>), with CODESET
 * following libixcompat's LC_CTYPE: "UTF-8" once setlocale chose a UTF-8
 * locale (locale.c), else "ISO8859-1". The strings are the C locale's. */
#include <langinfo.h>

extern int __ixc_ctype_utf8;

static const char *const days[] = { "Sunday", "Monday", "Tuesday",
    "Wednesday", "Thursday", "Friday", "Saturday" };
static const char *const mons[] = { "January", "February", "March",
    "April", "May", "June", "July", "August", "September", "October",
    "November", "December" };
static const char *const abdays[] = { "Sun", "Mon", "Tue", "Wed", "Thu",
    "Fri", "Sat" };
static const char *const abmons[] = { "Jan", "Feb", "Mar", "Apr", "May",
    "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

char *
nl_langinfo(nl_item item)
{
	switch (item) {
	case CODESET:	return (__ixc_ctype_utf8 ? "UTF-8" : "ISO8859-1");
	case D_T_FMT:	return ("%a %b %e %H:%M:%S %Y");
	case D_FMT:	return ("%m/%d/%y");
	case T_FMT:	return ("%H:%M:%S");
	case T_FMT_AMPM: return ("%I:%M:%S %p");
	case AM_STR:	return ("AM");
	case PM_STR:	return ("PM");
	case RADIXCHAR:	return (".");
	case THOUSEP:	return ("");
	case YESEXPR:	return ("^[yY]");
	case NOEXPR:	return ("^[nN]");
	case CRNCYSTR:	return ("-");
	}
	if (item >= DAY_1 && item <= DAY_7)
		return ((char *)days[item - DAY_1]);
	if (item >= MON_1 && item <= MON_12)
		return ((char *)mons[item - MON_1]);
	if (item >= ABDAY_1 && item <= ABDAY_7)
		return ((char *)abdays[item - ABDAY_1]);
	if (item >= ABMON_1 && item <= ABMON_12)
		return ((char *)abmons[item - ABMON_1]);
	if (item >= ALTMON_1 && item <= ALTMON_12)
		return ((char *)mons[item - ALTMON_1]);
	if (item >= ABALTMON_1 && item <= ABALTMON_12)
		return ((char *)abmons[item - ABALTMON_1]);
	return ("");	/* ERA, ALT_DIGITS, ...: none in the C locale */
}

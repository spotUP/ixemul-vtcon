/* nl_langinfo for ixemul's one locale (see <langinfo.h>). (UP-Term) */
#include <langinfo.h>

char *
nl_langinfo(nl_item item)
{
	switch (item) {
	case CODESET:	return "ISO8859-1";
	case D_T_FMT:	return "%a %b %e %H:%M:%S %Y";
	case D_FMT:	return "%m/%d/%y";
	case T_FMT:	return "%H:%M:%S";
	case RADIXCHAR:	return ".";
	case THOUSEP:	return "";
	case YESEXPR:	return "^[yY]";
	case NOEXPR:	return "^[nN]";
	}
	return "";
}

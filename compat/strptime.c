/*
 * Copyright (C) 2026 spotUP.
 * This file is part of ixemul-vtcon, a modified ixemul.library. It is free
 * software; you can redistribute it and/or modify it under the terms of the
 * GNU Lesser General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version. See LICENSES.README and COPYING.LIB.
 */
/* strptime (POSIX.1-2008), which ixemul has none of (jq 1.7.1 strptime/1).
 * The "C" locale only, as every other function of libixcompat.
 *
 * Supported: %% %a %A %b %B %h %c %C %d %D %e %F %H %I %j %m %M %n %t %p %r
 * %R %S %T %y %Y %z %Z, white space (matches any amount, also none), and
 * the numeric day-of-week and week-number conversions %u %w %U %V %W %g %G
 * (read and range checked, their value is not stored: they do not determine
 * a date by themselves, POSIX says the same of %U and %W). %E and %O
 * modifiers are accepted and mean the plain conversion (the "C" locale has
 * no alternative forms). Not supported, so the call returns NULL (it does
 * not pretend): %s, %k, %l and any unknown conversion.
 *
 * %z reads +hh, +hhmm, +hh:mm or Z and stores tm_gmtoff; %Z skips a time
 * zone name (letters) and stores nothing, as glibc does. When the year,
 * month and day are all known, tm_wday and tm_yday are filled in (glibc
 * does too; POSIX leaves them unspecified). Fields no conversion set are
 * not touched. On a mismatch the result is NULL and *tm may be partly
 * written. The host test builds this file with -DIXC_STRPTIME=ixc_strptime. */
#include <time.h>
#include <string.h>
#include <ctype.h>

#ifndef IXC_STRPTIME
#define IXC_STRPTIME strptime
#endif

static const char *const day_full[7] = {
	"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday" };
static const char *const mon_full[12] = {
	"January", "February", "March", "April", "May", "June", "July",
	"August", "September", "October", "November", "December" };

static int
lc(int c)
{
	return (c >= 'A' && c <= 'Z') ? c + 32 : c;
}

/* case-insensitive prefix match: strlen(name) bytes */
static int
prefix(const char *s, const char *name, size_t n)
{
	size_t i;

	for (i = 0; i < n; i++)
		if (!s[i] || lc((unsigned char)s[i]) != lc((unsigned char)name[i]))
			return 0;
	return 1;
}

/* a full or an abbreviated (3 letter) name from tab[0..n-1]; the full name wins */
static const char *
name(const char *s, const char *const *tab, int n, int *idx)
{
	int i;

	for (i = 0; i < n; i++)
		if (prefix(s, tab[i], strlen(tab[i]))) {
			*idx = i;
			return s + strlen(tab[i]);
		}
	for (i = 0; i < n; i++)
		if (prefix(s, tab[i], 3)) {
			*idx = i;
			return s + 3;
		}
	return NULL;
}

/* at most maxd digits (leading white space skipped) with lo <= value <= hi */
static const char *
num(const char *s, int maxd, int lo, int hi, int *out)
{
	int v = 0, d = 0;

	while (isspace((unsigned char)*s))
		s++;
	while (d < maxd && *s >= '0' && *s <= '9') {
		v = v * 10 + (*s - '0');
		s++;
		d++;
	}
	if (d == 0 || v < lo || v > hi)
		return NULL;
	*out = v;
	return s;
}

static int
leap(int y)
{
	return (y % 4 == 0 && y % 100 != 0) || y % 400 == 0;
}

static const short cum[12] = { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 };

/* tm_yday from month and day, tm_wday from the date (Sakamoto, proleptic Gregorian) */
static void
derive(struct tm *tm)
{
	static const int t[12] = { 0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4 };
	int y = tm->tm_year + 1900;

	tm->tm_yday = cum[tm->tm_mon] + (tm->tm_mon > 1 && leap(y) ? 1 : 0) + tm->tm_mday - 1;
	if (tm->tm_mon < 2)
		y--;
	/* floor division so years before 1 (y negative) come out right */
	{
		int q4 = y >= 0 ? y / 4 : -((3 - y) / 4);
		int q100 = y >= 0 ? y / 100 : -((99 - y) / 100);
		int q400 = y >= 0 ? y / 400 : -((399 - y) / 400);
		int w = (y + q4 - q100 + q400 + t[tm->tm_mon] + tm->tm_mday) % 7;

		tm->tm_wday = w < 0 ? w + 7 : w;
	}
}

char *
IXC_STRPTIME(const char *buf, const char *fmt, struct tm *tm)
{
	const char *s = buf;
	int have_y = 0, have_m = 0, have_d = 0, have_yday = 0, century = -1, yy = -1;
	int pm = -1, hour12 = 0, v, i;

	for (; *fmt; fmt++) {
		if (isspace((unsigned char)*fmt)) {
			while (isspace((unsigned char)*s))
				s++;
			continue;
		}
		if (*fmt != '%') {
			if (*s != *fmt)
				return NULL;
			s++;
			continue;
		}
		fmt++;
		if (*fmt == 'E' || *fmt == 'O')
			fmt++;
		switch (*fmt) {
		case '%':
			if (*s != '%')
				return NULL;
			s++;
			break;
		case 'a': case 'A':
			if (!(s = name(s, day_full, 7, &v)))
				return NULL;
			tm->tm_wday = v;
			break;
		case 'b': case 'B': case 'h':
			if (!(s = name(s, mon_full, 12, &v)))
				return NULL;
			tm->tm_mon = v;
			have_m = 1;
			break;
		case 'c':
			if (!(s = IXC_STRPTIME(s, "%a %b %e %H:%M:%S %Y", tm)))
				return NULL;
			have_y = have_m = have_d = 1;
			break;
		case 'C':
			if (!(s = num(s, 2, 0, 99, &century)))
				return NULL;
			break;
		case 'd': case 'e':
			if (!(s = num(s, 2, 1, 31, &tm->tm_mday)))
				return NULL;
			have_d = 1;
			break;
		case 'D':
			if (!(s = IXC_STRPTIME(s, "%m/%d/%y", tm)))
				return NULL;
			have_y = have_m = have_d = 1;
			yy = -1;
			break;
		case 'F':
			if (!(s = IXC_STRPTIME(s, "%Y-%m-%d", tm)))
				return NULL;
			have_y = have_m = have_d = 1;
			break;
		case 'H':
			if (!(s = num(s, 2, 0, 23, &tm->tm_hour)))
				return NULL;
			hour12 = 0;
			break;
		case 'I':
			if (!(s = num(s, 2, 1, 12, &v)))
				return NULL;
			tm->tm_hour = v % 12;
			hour12 = 1;
			break;
		case 'j':
			if (!(s = num(s, 3, 1, 366, &v)))
				return NULL;
			tm->tm_yday = v - 1;
			have_yday = 1;
			break;
		case 'm':
			if (!(s = num(s, 2, 1, 12, &v)))
				return NULL;
			tm->tm_mon = v - 1;
			have_m = 1;
			break;
		case 'M':
			if (!(s = num(s, 2, 0, 59, &tm->tm_min)))
				return NULL;
			break;
		case 'n': case 't':
			while (isspace((unsigned char)*s))
				s++;
			break;
		case 'p':
			if (prefix(s, "AM", 2))
				pm = 0;
			else if (prefix(s, "PM", 2))
				pm = 1;
			else
				return NULL;
			s += 2;
			break;
		case 'r':
			if (!(s = IXC_STRPTIME(s, "%I:%M:%S %p", tm)))
				return NULL;
			hour12 = 1;
			break;
		case 'R':
			if (!(s = IXC_STRPTIME(s, "%H:%M", tm)))
				return NULL;
			break;
		case 'S':
			if (!(s = num(s, 2, 0, 61, &tm->tm_sec)))
				return NULL;
			break;
		case 'T':
			if (!(s = IXC_STRPTIME(s, "%H:%M:%S", tm)))
				return NULL;
			break;
		case 'u':
			if (!(s = num(s, 1, 1, 7, &v)))
				return NULL;
			tm->tm_wday = v % 7;
			break;
		case 'w':
			if (!(s = num(s, 1, 0, 6, &tm->tm_wday)))
				return NULL;
			break;
		case 'U': case 'W': case 'V':
			if (!(s = num(s, 2, *fmt == 'V' ? 1 : 0, 53, &v)))
				return NULL;
			break;
		case 'g':
			if (!(s = num(s, 2, 0, 99, &v)))
				return NULL;
			break;
		case 'G':
			if (!(s = num(s, 4, 0, 9999, &v)))
				return NULL;
			break;
		case 'y':
			if (!(s = num(s, 2, 0, 99, &yy)))
				return NULL;
			have_y = 1;
			break;
		case 'Y':
			if (!(s = num(s, 4, 0, 9999, &v)))
				return NULL;
			tm->tm_year = v - 1900;
			have_y = 1;
			century = yy = -1;
			break;
		case 'z': {
			int sign = 1, hh, mm = 0;

			while (isspace((unsigned char)*s))
				s++;
			if (*s == 'Z') {
				s++;
				tm->tm_gmtoff = 0;
				break;
			}
			if (*s == '+')
				s++;
			else if (*s == '-') {
				sign = -1;
				s++;
			} else
				return NULL;
			if (!(s = num(s, 2, 0, 23, &hh)))
				return NULL;
			if (*s == ':')
				s++;
			if (*s >= '0' && *s <= '9' && !(s = num(s, 2, 0, 59, &mm)))
				return NULL;
			tm->tm_gmtoff = sign * (hh * 3600L + mm * 60L);
			break;
		}
		case 'Z':
			for (i = 0; isalpha((unsigned char)s[i]); i++)
				;
			if (i == 0)
				return NULL;
			s += i;
			break;
		default:			/* %s, %k, %l, unknown: not supported */
			return NULL;
		}
	}
	if (pm >= 0 && hour12)
		tm->tm_hour += pm ? 12 : 0;
	if (yy >= 0) {
		/* POSIX: 69-99 -> 1969-1999, 00-68 -> 2000-2068; %C overrides the guess */
		int c = century >= 0 ? century : (yy >= 69 ? 19 : 20);

		tm->tm_year = c * 100 + yy - 1900;
	} else if (century >= 0 && !have_y)
		tm->tm_year = century * 100 - 1900;
	if (have_y && have_m && have_d)
		derive(tm);
	else if (have_y && have_yday && !have_m) {
		/* %j with a year: month and day from the day of the year */
		int y = tm->tm_year + 1900, d = tm->tm_yday, m = 11;

		if (d >= (leap(y) ? 366 : 365))
			return NULL;
		while (m > 0 && d < cum[m] + (m > 1 && leap(y) ? 1 : 0))
			m--;
		tm->tm_mon = m;
		tm->tm_mday = d - cum[m] - (m > 1 && leap(y) ? 1 : 0) + 1;
		derive(tm);
	}
	return (char *)s;
}

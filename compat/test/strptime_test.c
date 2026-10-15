/*
 * Copyright (C) 2026 spotUP.
 * This file is part of ixemul-vtcon, a modified ixemul.library. It is free
 * software; you can redistribute it and/or modify it under the terms of the
 * GNU Lesser General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version. See LICENSES.README and COPYING.LIB.
 */
/* Host test of compat/strptime.c (built with -DIXC_STRPTIME=ixc_strptime):
 * the formats jq's strptime/1 and the common POSIX set against fixed
 * expectations, the end pointer, the rejected input and the unsupported
 * conversions (NULL, never a made-up value). Where the host's own strptime
 * agrees on the plain ISO forms, the two are compared as well. Exit
 * status = failures. */
#include <stdio.h>
#include <string.h>
#include <time.h>

char *ixc_strptime(const char *, const char *, struct tm *);

static int fails;

#define CHECK(cond, ...) do { if (!(cond)) { printf("[FAIL] " __VA_ARGS__); printf("\n"); fails++; } } while (0)

/* parse s with fmt from a zeroed tm; want "Y-M-D h:m:s wday yday" and end offset (-1: NULL) */
static void
ok(const char *s, const char *fmt, const char *want, int end)
{
	struct tm tm;
	char got[80], *r;

	memset(&tm, 0, sizeof tm);
	r = ixc_strptime(s, fmt, &tm);
	if (end < 0) {
		CHECK(r == NULL, "strptime(\"%s\", \"%s\") = \"%s\", want NULL", s, fmt, r ? r : "");
		return;
	}
	CHECK(r != NULL && r - s == end, "strptime(\"%s\", \"%s\") end %ld, want %d", s, fmt, r ? (long)(r - s) : -1L, end);
	snprintf(got, sizeof got, "%d-%d-%d %d:%d:%d %d %d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
	    tm.tm_hour, tm.tm_min, tm.tm_sec, tm.tm_wday, tm.tm_yday);
	CHECK(!strcmp(got, want), "strptime(\"%s\", \"%s\") -> %s, want %s", s, fmt, got, want);
}

int
main(void)
{
	struct tm tm, h;
	const char *iso = "2015-03-05T23:51:47Z";

	/* jq: strptime("%Y-%m-%dT%H:%M:%SZ"), wday and yday filled in */
	ok(iso, "%Y-%m-%dT%H:%M:%SZ", "2015-3-5 23:51:47 4 63", 20);
	ok("2024-02-29T00:00:00Z", "%Y-%m-%dT%H:%M:%SZ", "2024-2-29 0:0:0 4 59", 20);
	ok("2000-12-31T12:00:00Z", "%Y-%m-%dT%H:%M:%SZ", "2000-12-31 12:0:0 0 365", 20);
	ok("1999-01-01T00:00:00Z", "%Y-%m-%dT%H:%M:%SZ", "1999-1-1 0:0:0 5 0", 20);
	ok("1900-03-01", "%F", "1900-3-1 0:0:0 4 59", 10);	/* 1900 is not a leap year */
	/* the shorthand forms */
	ok("2015-03-05 23:51:47", "%F %T", "2015-3-5 23:51:47 4 63", 19);
	ok("03/05/15", "%D", "2015-3-5 0:0:0 4 63", 8);
	ok("03/05/70", "%D", "1970-3-5 0:0:0 4 63", 8);	/* %y 69..99 is 19xx */
	ok("03/05/68", "%D", "2068-3-5 0:0:0 1 64", 8);	/* 00..68 is 20xx */
	ok("19 15-03-05", "%C %y-%m-%d", "1915-3-5 0:0:0 5 63", 11);
	ok("23:51", "%R", "1900-1-0 23:51:0 0 0", 5);
	/* names, any case, full or abbreviated */
	ok("Thu Mar  5 23:51:47 2015", "%a %b %e %H:%M:%S %Y", "2015-3-5 23:51:47 4 63", 24);
	ok("thursday, MARCH 5 2015", "%A, %B %d %Y", "2015-3-5 0:0:0 4 63", 22);
	ok("Sep 9 2015", "%h %d %Y", "2015-9-9 0:0:0 3 251", 10);
	ok("Thu Mar  5 23:51:47 2015", "%c", "2015-3-5 23:51:47 4 63", 24);
	/* 12 hour clock */
	ok("12:00:00 AM", "%r", "1900-1-0 0:0:0 0 0", 11);
	ok("12:30 PM", "%I:%M %p", "1900-1-0 12:30:0 0 0", 8);
	ok("01:05:09 pm", "%r", "1900-1-0 13:5:9 0 0", 11);
	ok("11 PM", "%I %p", "1900-1-0 23:0:0 0 0", 5);
	/* day of the year */
	ok("2024 366", "%Y %j", "2024-12-31 0:0:0 2 365", 8);
	ok("2015 064", "%Y %j", "2015-3-5 0:0:0 4 63", 8);
	ok("2015 366", "%Y %j", "", -1);	/* 2015 has 365 days */
	ok("2016 366", "%Y %j", "2016-12-31 0:0:0 6 365", 8);
	ok("0000-01-01", "%F", "0-1-1 0:0:0 6 0", 10);	/* wday of year 0 Jan/Feb: floor division */
	ok("2024-06-15", "%F", "2024-6-15 0:0:0 6 166", 10);
	/* white space, literals, %%, %n %t, the end pointer stops at unused text */
	ok("2015   03\t05", "%Y %m %d", "2015-3-5 0:0:0 4 63", 12);
	ok("2015-03-05trailing", "%Y-%m-%d", "2015-3-5 0:0:0 4 63", 10);
	ok("10%", "%H%%", "1900-1-0 10:0:0 0 0", 3);
	/* numeric weekday and week number: read, range checked */
	ok("4", "%w", "1900-1-0 0:0:0 4 0", 1);
	ok("7", "%u", "1900-1-0 0:0:0 0 0", 1);
	ok("09", "%U", "1900-1-0 0:0:0 0 0", 2);
	/* %E and %O modifiers are the plain conversions */
	ok("2015-03-05", "%EY-%Om-%Od", "2015-3-5 0:0:0 4 63", 10);
	/* rejected input */
	ok("2015-13-05", "%Y-%m-%d", "", -1);
	ok("2015-00-05", "%Y-%m-%d", "", -1);
	ok("2015-03-32", "%Y-%m-%d", "", -1);
	ok("2015-03-05T24:00:00Z", "%Y-%m-%dT%H:%M:%SZ", "", -1);
	ok("2015-03-05T23:60:00Z", "%Y-%m-%dT%H:%M:%SZ", "", -1);
	ok("2015-03-05T23:51:62Z", "%Y-%m-%dT%H:%M:%SZ", "", -1);
	ok("2015-03-05T23:51:47", "%Y-%m-%dT%H:%M:%SZ", "", -1);	/* missing Z */
	ok("2015/03/05", "%Y-%m-%d", "", -1);
	ok("", "%Y", "", -1);
	ok("abc", "%d", "", -1);
	ok("Foo", "%a", "", -1);
	ok("13 PM", "%I %p", "", -1);
	ok("2015 367", "%Y %j", "", -1);
	ok("5", "%u%u", "", -1);
	ok("8", "%u", "", -1);
	/* unsupported conversions return NULL, not a value */
	ok("1425599507", "%s", "", -1);
	ok("5", "%k", "", -1);
	ok("5", "%l", "", -1);
	ok("5", "%Q", "", -1);
	ok("5", "%", "", -1);

	/* %z and %Z */
	memset(&tm, 0, sizeof tm);
	CHECK(ixc_strptime("+0530", "%z", &tm) && tm.tm_gmtoff == 19800, "%%z +0530 -> %ld", tm.tm_gmtoff);
	CHECK(ixc_strptime("-08:00", "%z", &tm) && tm.tm_gmtoff == -28800, "%%z -08:00 -> %ld", tm.tm_gmtoff);
	CHECK(ixc_strptime("+02", "%z", &tm) && tm.tm_gmtoff == 7200, "%%z +02 -> %ld", tm.tm_gmtoff);
	CHECK(ixc_strptime("Z", "%z", &tm) && tm.tm_gmtoff == 0, "%%z Z");
	CHECK(ixc_strptime("0530", "%z", &tm) == NULL, "%%z without a sign");
	CHECK(ixc_strptime("+2500", "%z", &tm) == NULL, "%%z hour 25");
	CHECK(ixc_strptime("UTC rest", "%Z", &tm) && !strcmp(ixc_strptime("UTC rest", "%Z", &tm), " rest"), "%%Z skips the name");
	CHECK(ixc_strptime("123", "%Z", &tm) == NULL, "%%Z needs letters");

	/* the host's strptime agrees on the ISO form (a differential check) */
	memset(&tm, 0, sizeof tm);
	memset(&h, 0, sizeof h);
	CHECK(ixc_strptime(iso, "%Y-%m-%dT%H:%M:%SZ", &tm) != NULL && strptime(iso, "%Y-%m-%dT%H:%M:%SZ", &h) != NULL, "host compare parse");
	CHECK(tm.tm_year == h.tm_year && tm.tm_mon == h.tm_mon && tm.tm_mday == h.tm_mday && tm.tm_hour == h.tm_hour &&
	    tm.tm_min == h.tm_min && tm.tm_sec == h.tm_sec && tm.tm_wday == h.tm_wday && tm.tm_yday == h.tm_yday,
	    "ISO parse differs from the host's strptime");

	if (fails)
		return 1;
	puts("[OK] strptime");
	return 0;
}

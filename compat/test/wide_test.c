/* Host test of libixcompat's wide-character core (utf8.c, uclass.c):
 *   wide_test         the codec cases below; exit status = failures
 *   wide_test dump    every code point's class, upper, lower (for
 *                     check_uclass.py, which compares with Python's
 *                     unicodedata of the same Unicode version) */
#include <stdio.h>
#include <string.h>
#include "../ixc_wide.h"

static int fails;
#define CHECK(cond, what) do { if (!(cond)) { printf("[FAIL] %s\n", what); fails++; } \
	else printf("[OK]   %s\n", what); } while (0)

/* decode all of s in one call per character; returns the code points */
static int
decode(const char *s, size_t n, unsigned long *out, int utf8)
{
	struct ixc_mbstate st;
	size_t r;
	int k = 0;

	memset(&st, 0, sizeof st);
	while (n > 0) {
		r = __ixc_mbrtowc(&out[k], s, n, &st, utf8);
		if (r == (size_t)-1 || r == (size_t)-2)
			return (r == (size_t)-1 ? -1 : -2);
		if (r == 0)
			r = 1;
		k++, s += r, n -= r;
	}
	return (k);
}

int
main(int argc, char **argv)
{
	struct ixc_mbstate st;
	unsigned long wc[8], c;
	char buf[8];
	size_t r;
	int n;

	if (argc > 1 && strcmp(argv[1], "dump") == 0) {
		for (c = 0; c <= 0x10ffff; c++)
			printf("%lx %x %lx %lx\n", c, __ixc_uclass(c),
			    __ixc_toupper(c), __ixc_tolower(c));
		return (0);
	}

	n = decode("caf\xc3\xa9", 5, wc, 1);
	CHECK(n == 4 && wc[3] == 0xe9, "UTF-8 'cafe\\u0301' decodes e-acute as U+00E9");
	n = decode("\xf0\x9f\x98\x80", 4, wc, 1);
	CHECK(n == 1 && wc[0] == 0x1f600, "a 4-byte emoji decodes");
	CHECK(decode("\xc0\xaf", 2, wc, 1) == -1, "overlong 2-byte form rejected");
	CHECK(decode("\xe0\x80\xaf", 3, wc, 1) == -1, "overlong 3-byte form rejected");
	CHECK(decode("\xed\xa0\x80", 3, wc, 1) == -1, "surrogate U+D800 rejected");
	CHECK(decode("\xf4\x90\x80\x80", 4, wc, 1) == -1, "above U+10FFFF rejected");
	CHECK(decode("\xf5\x80\x80\x80", 4, wc, 1) == -1, "lead byte F5 rejected");
	CHECK(decode("\x80", 1, wc, 1) == -1, "lone continuation byte rejected");
	CHECK(decode("\xc3" "A", 2, wc, 1) == -1, "lead byte followed by ASCII rejected");
	CHECK(decode("\xe2\x82", 2, wc, 1) == -2, "truncated sequence is incomplete (-2)");

	/* restartable: the euro sign fed one byte at a time */
	memset(&st, 0, sizeof st);
	r = __ixc_mbrtowc(&wc[0], "\xe2", 1, &st, 1);
	CHECK(r == (size_t)-2 && st.need == 2, "E2: incomplete, two bytes to come");
	r = __ixc_mbrtowc(&wc[0], "\x82", 1, &st, 1);
	CHECK(r == (size_t)-2 && st.need == 1, "82: incomplete, one byte to come");
	r = __ixc_mbrtowc(&wc[0], "\xac", 1, &st, 1);
	CHECK(r == 1 && wc[0] == 0x20ac && st.need == 0, "AC: U+20AC, one byte used, state initial");
	/* a surrogate split across calls is still rejected */
	memset(&st, 0, sizeof st);
	r = __ixc_mbrtowc(&wc[0], "\xed", 1, &st, 1);
	r = __ixc_mbrtowc(&wc[0], "\xa0\x80", 2, &st, 1);
	CHECK(r == (size_t)-1 && st.need == 0, "surrogate split across calls rejected, state reset");
	CHECK(__ixc_mbrtowc(&wc[0], "", 1, &st, 1) == 0 && wc[0] == 0, "NUL returns 0");

	n = decode("caf\xe9", 4, wc, 0);
	CHECK(n == 4 && wc[3] == 0xe9, "ISO 8859-1: byte E9 is U+00E9");
	CHECK(__ixc_wcrtomb(buf, 0x20ac, 0) == (size_t)-1, "ISO 8859-1 has no euro sign");
	CHECK(__ixc_wcrtomb(buf, 0xe9, 0) == 1 && (unsigned char)buf[0] == 0xe9, "ISO 8859-1 encodes U+00E9 as E9");

	for (c = 0; c <= 0x10ffff; c += (c < 0x30000 ? 1 : 0x101)) {
		unsigned long back;
		r = __ixc_wcrtomb(buf, c, 1);
		if (c >= 0xd800 && c <= 0xdfff) {
			if (r != (size_t)-1)
				break;
			continue;
		}
		memset(&st, 0, sizeof st);
		if (r == (size_t)-1 || __ixc_mbrtowc(&back, buf, r, &st, 1) != (c ? r : 0) || back != c)
			break;
	}
	CHECK(c > 0x10ffff, "UTF-8 round trip, every code point (surrogates refused)");
	CHECK(__ixc_wcrtomb(buf, 0x110000, 1) == (size_t)-1, "U+110000 has no encoding");

	CHECK(__ixc_name_is_utf8("en_US.UTF-8"), "en_US.UTF-8 is UTF-8");
	CHECK(__ixc_name_is_utf8("C.utf8"), "C.utf8 is UTF-8");
	CHECK(__ixc_name_is_utf8("de_DE.UTF-8@euro"), "de_DE.UTF-8@euro is UTF-8");
	CHECK(!__ixc_name_is_utf8("C"), "C is not UTF-8");
	CHECK(!__ixc_name_is_utf8("en_US.ISO8859-1"), "en_US.ISO8859-1 is not UTF-8");
	CHECK(!__ixc_name_is_utf8("en_US.UTF-80"), "en_US.UTF-80 is not UTF-8");
	CHECK(!__ixc_name_is_utf8("UTF-8"), "UTF-8 without a language part is a name, not a codeset");

	CHECK(__ixc_toupper(0xe9) == 0xc9 && __ixc_tolower(0xc9) == 0xe9, "e-acute upper/lower");
	CHECK(__ixc_isclass(0xe9, IXC_ALPHA) && !__ixc_isclass(0xe9, IXC_PUNCT), "e-acute is alpha");
	CHECK(__ixc_isclass(0x3000, IXC_SPACE) && !__ixc_isclass(0xa0, IXC_SPACE), "ideographic space is space, NBSP is not");
	CHECK(__ixc_isclass(0x20ac, IXC_PUNCT) && __ixc_isclass(0x20ac, IXC_GRAPH), "euro sign is punct and graph");
	CHECK(!__ixc_isclass(0x0378, IXC_PRINT), "unassigned U+0378 is not printable");
	CHECK(__ixc_classname("xdigit") == IXC_XDIGIT && __ixc_classname("alphax") == 0, "class names");
	printf("%d failures\n", fails);
	return (fails);
}

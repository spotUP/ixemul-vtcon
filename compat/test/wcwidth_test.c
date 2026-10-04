/* Host test of compat/wcwidth.c: vtcon's table through the POSIX rules
 * (NUL 0, controls/surrogates/out of range -1). Exit status = failures. */
#include <stdio.h>
#include <wchar.h>

static int fails;
#define CHECK(cond, what) do { if (!(cond)) { printf("[FAIL] %s\n", what); fails++; } \
	else printf("[OK]   %s\n", what); } while (0)

int
main(void)
{
	static const wchar_t mix[] = { 'a', 0x4e2d, 0x0301, 0 };
	static const wchar_t ctl[] = { 'a', '\n', 0 };

	CHECK(wcwidth(0) == 0, "NUL has width 0");
	CHECK(wcwidth('\n') == -1 && wcwidth(0x7f) == -1 && wcwidth(0x9b) == -1, "C0, DEL, C1 have no width");
	CHECK(wcwidth('a') == 1 && wcwidth(0xe9) == 1, "ASCII and Latin-1 letters are 1");
	CHECK(wcwidth(0x0301) == 0, "a combining acute is 0");
	CHECK(wcwidth(0x4e2d) == 2 && wcwidth(0x1f600) == 2, "CJK and emoji are 2");
	CHECK(wcwidth(0xad) == 1, "soft hyphen is 1 (glibc's rule)");
	CHECK(wcwidth(0xd800) == -1 && wcwidth(0x110000) == -1, "surrogate and > U+10FFFF have no width");
	CHECK(wcswidth(mix, 3) == 3, "wcswidth: a + CJK + combining = 3");
	CHECK(wcswidth(mix, 1) == 1, "wcswidth stops at n");
	CHECK(wcswidth(ctl, 2) == -1, "wcswidth of a string with a control is -1");
	printf("%d failures\n", fails);
	return (fails);
}

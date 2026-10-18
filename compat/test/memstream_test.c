/*
 * Copyright (C) 2026 spotUP.
 * This file is part of ixemul-vtcon, a modified ixemul.library. It is free
 * software; you can redistribute it and/or modify it under the terms of the
 * GNU Lesser General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version. See LICENSES.README and COPYING.LIB.
 */
/* fmemopen and open_memstream (compat/memstream.c). Host: compiled with
 * -DIXC_MEMSTREAM, names ixc_fmemopen/ixc_open_memstream; Amiga: the real
 * names from libixcompat.a through <stdio.h>. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#ifdef IXC_MEMSTREAM
FILE *ixc_fmemopen(void *, size_t, const char *);
FILE *ixc_open_memstream(char **, size_t *);
#define T_FMEMOPEN ixc_fmemopen
#define T_OPEN_MEMSTREAM ixc_open_memstream
#else
#define T_FMEMOPEN fmemopen
#define T_OPEN_MEMSTREAM open_memstream
#endif

static int fails;
#define CHECK(c) do { if (!(c)) { printf("[FAIL] line %d: %s (errno %d)\n", __LINE__, #c, errno); fails++; } } while (0)

int
main(void)
{
	char buf[32], line[32], big[200];
	char *p = NULL;
	size_t sz = 99, i;
	FILE *f;
	int ch;

	/* fmemopen read: line reads, EOF at size, seek */
	strcpy(buf, "one\ntwo\nthree");
	f = T_FMEMOPEN(buf, 13, "r");
	CHECK(f != NULL);
	CHECK(fgets(line, sizeof line, f) && strcmp(line, "one\n") == 0);
	CHECK(fgets(line, sizeof line, f) && strcmp(line, "two\n") == 0);
	CHECK(fgets(line, sizeof line, f) && strcmp(line, "three") == 0);
	CHECK(fgets(line, sizeof line, f) == NULL && feof(f));
	CHECK(fseek(f, 4, SEEK_SET) == 0 && (ch = fgetc(f)) == 't');
	CHECK(fclose(f) == 0);

	/* fmemopen write: truncates, NUL-terminates after flush, stops at size */
	memset(buf, 'z', sizeof buf);
	f = T_FMEMOPEN(buf, 8, "w");
	CHECK(f != NULL && buf[0] == 0);
	fputs("abc", f);
	fflush(f);
	CHECK(strcmp(buf, "abc") == 0);
	CHECK(fputs("0123456789", f) >= 0);
	fflush(f);
	CHECK(ferror(f) || memcmp(buf, "abc01234", 8) == 0);
	fclose(f);
	CHECK(buf[8] == 'z');

	/* append starts at the first NUL; a+ reads from the start */
	strcpy(buf, "head");
	f = T_FMEMOPEN(buf, sizeof buf, "a+");
	CHECK(f != NULL);
	fputs("-tail", f);
	fflush(f);
	CHECK(strcmp(buf, "head-tail") == 0);
	rewind(f);
	CHECK(fgets(line, sizeof line, f) && strcmp(line, "head-tail") == 0);
	fclose(f);

	/* NULL buffer needs a + mode and is freed by fclose */
	CHECK(T_FMEMOPEN(NULL, 16, "r") == NULL && errno == EINVAL);
	f = T_FMEMOPEN(NULL, 16, "w+");
	CHECK(f != NULL);
	fputs("hello", f);
	rewind(f);
	CHECK(fgets(line, sizeof line, f) && strcmp(line, "hello") == 0);
	fclose(f);
	CHECK(T_FMEMOPEN(buf, 0, "r") == NULL);
	CHECK(T_FMEMOPEN(buf, 4, "q") == NULL);

	/* open_memstream: pointer and size valid after fflush, NUL-terminated */
	f = T_OPEN_MEMSTREAM(&p, &sz);
	CHECK(f != NULL && sz == 0);
	fprintf(f, "x=%d", 42);
	fflush(f);
	CHECK(p != NULL && sz == 4 && strcmp(p, "x=42") == 0 && p[sz] == 0);
	for (i = 0; i < 100; i++)
		fputs("0123456789", f);		/* grows well past the first allocation */
	fclose(f);
	CHECK(sz == 4 + 1000 && p[sz] == 0 && strncmp(p, "x=420123456789", 14) == 0);
	free(p);

	/* seek back: size is min(position, length), data intact */
	p = NULL;
	f = T_OPEN_MEMSTREAM(&p, &sz);
	fputs("abcdef", f);
	fflush(f);
	CHECK(sz == 6);
	CHECK(fseek(f, 2, SEEK_SET) == 0);
	fputc('X', f);
	fflush(f);
	CHECK(sz == 3);
	fclose(f);
	CHECK(sz == 3 && memcmp(p, "abX", 3) == 0 && strcmp(p, "abXdef") == 0);
	free(p);

	/* a seek past the end zero-fills the gap */
	p = NULL;
	f = T_OPEN_MEMSTREAM(&p, &sz);
	fputs("ab", f);
	fseek(f, 5, SEEK_SET);
	fputc('c', f);
	fclose(f);
	CHECK(sz == 6 && memcmp(p, "ab\0\0\0c", 6) == 0 && p[6] == 0);
	free(p);

	(void)big;
	printf(fails ? "[FAIL] memstream: %d\n" : "[OK] memstream\n", fails);
	return fails != 0;
}

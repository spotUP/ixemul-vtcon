/*
 * Copyright (C) 2026 spotUP.
 * This file is part of ixemul-vtcon, a modified ixemul.library. It is free
 * software; you can redistribute it and/or modify it under the terms of the
 * GNU Lesser General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version. See LICENSES.README and COPYING.LIB.
 */
/* Host test of library/vol_path.c: "Vol:/x" names Vol:x, so "$HOME/x" with
 * HOME a volume root ("System:") opens System:x. Run: make -C tests/host test */
#include <stdio.h>
#include <string.h>
#include "../../library/vol_path.h"

static int failed, run;

static void check(const char *in, const char *want)
{
    char buf[256];

    run++;
    strcpy(buf, in);
    ix_vol_slashes(buf);
    if (strcmp(buf, want)) {
        failed++;
        printf("FAIL %s: got %s, want %s\n", in, buf, want);
    }
}

int main(void)
{
    check("System:/.tutorial", "System:.tutorial");    /* $HOME/.tutorial */
    check("Vol://a/b", "Vol:a/b");
    check("Vol:/", "Vol:");
    check("Vol:a/b", "Vol:a/b");                       /* unchanged */
    check("Vol:", "Vol:");
    check(":/x", ":x");                                /* the current volume */
    check("a/b:/c", "a/b:/c");                         /* no volume: a slash first */
    check("plain/name", "plain/name");
    check("", "");
    printf("%d checks, %d failed\n", run, failed);
    return failed != 0;
}

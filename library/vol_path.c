/*
 * Copyright (C) 2026 spotUP.
 * This file is part of ixemul-vtcon, a modified ixemul.library. It is free
 * software; you can redistribute it and/or modify it under the terms of the
 * GNU Lesser General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version. See LICENSES.README and COPYING.LIB.
 */

/* To AmigaDOS a slash right after the colon is the parent of the volume's
 * root, always an error. POSIX code writes "$HOME/x", and HOME is a volume
 * root on the Amiga ("System:", vtcon shell/vsh.c canonical_home), so
 * "$HOME/.tutorial" came out "System:/.tutorial" and every ixemul program
 * failed on it (UP-Term, 2026-10-19). vsh maps the same names the same way
 * (vtcon shell/sh_exec.c sh_vol_slashes). Nothing that worked changes: the
 * Amiga reading of such a name never opened anything. */

#include "vol_path.h"

void ix_vol_slashes(char *name)
{
  char *p, *rest;

  for (p = name; *p && *p != ':'; p++)
    if (*p == '/')
      return;           /* a slash before any colon: no volume */
  if (*p != ':' || p[1] != '/')
    return;
  rest = p + 1;
  while (*rest == '/')
    rest++;
  p++;
  while ((*p++ = *rest++) != '\0')
    ;
}

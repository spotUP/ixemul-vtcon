/*
 * Copyright (C) 2026 spotUP.
 * This file is part of ixemul-vtcon, a modified ixemul.library. It is free
 * software; you can redistribute it and/or modify it under the terms of the
 * GNU Lesser General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version. See LICENSES.README and COPYING.LIB.
 */
/* vol_path.h -- "Vol:/x" names Vol:x (vol_path.c; used by __plock.c's
 * unslashify, tested on the host by tests/host/test_vol_path.c). No ixemul
 * headers: it builds anywhere. */
#ifndef IX_VOL_PATH_H
#define IX_VOL_PATH_H

/* Drops the slashes right after a volume's colon, in place: "System:/.x"
 * becomes "System:.x", "Vol://a/b" "Vol:a/b". A colon after a slash
 * ("a/b:c") is no volume, so the name is left as it is. */
void ix_vol_slashes(char *name);

#endif

# Copyright (C) 2026 spotUP.
# This file is part of ixemul-vtcon, a modified ixemul.library. It is free
# software; you can redistribute it and/or modify it under the terms of the
# GNU Lesser General Public License as published by the Free Software
# Foundation; either version 2 of the License, or (at your option) any later
# version. See LICENSES.README and COPYING.LIB.
# Host tests of ixemul's pure functions (no Amiga, no Docker): make test
CC ?= cc
CFLAGS = -std=c99 -Wall -Wextra -Werror -g -fsanitize=address,undefined
OUT = ../../build-host

test: $(OUT)/test_cli_args
	$(OUT)/test_cli_args

$(OUT)/test_cli_args: test_cli_args.c ../../library/cli_args.c ../../library/cli_args.h
	@mkdir -p $(OUT)
	$(CC) $(CFLAGS) -o $@ test_cli_args.c ../../library/cli_args.c

.PHONY: test

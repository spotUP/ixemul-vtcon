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

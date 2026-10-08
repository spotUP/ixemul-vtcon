/* Host test of library/cli_args.c, the AmigaDOS argument line splitter
 * _cli_parse.c gives main() its argv with. The expected argvs are what
 * dos.library 40's ReadItem returned for the same lines (vtcon
 * tests/amiga/readitem.c on the rig, 2026-10-08), except where cli_args.c
 * says ixemul differs on purpose (=, a newline inside quotes, an
 * unterminated quote). Run: make -C tests/host test */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../library/cli_args.h"

static int failed, run;

/* line -> the arguments, joined with | and quoted ones marked with Q: */
static void check(const char *name, const char *line, const char *want)
{
    char buf[512], got[512], *cur = buf, *end, *arg;
    int quoted;
    size_t len = strlen(line);

    memcpy(buf, line, len + 1);
    end = __ix_cli_line_end(buf, (long)len);
    *end = 0;
    got[0] = 0;
    while (__ix_cli_next_arg(&cur, end, &arg, &quoted)) {
        if (got[0])
            strcat(got, "|");
        if (quoted)
            strcat(got, "Q:");
        strcat(got, arg);
    }
    run++;
    if (strcmp(got, want)) {
        failed++;
        printf("FAIL %s\n  line [%s]\n  want [%s]\n  got  [%s]\n", name, line, want, got);
    }
}

int main(void)
{
    /* the * escapes in quotes, as ReadItem */
    check("star_quote", "\"a*\"b\"\n", "Q:a\"b");
    check("star_star", "\"c**d\"\n", "Q:c*d");
    check("star_newline", "\"x*Ny\" \"x*ny\"\n", "Q:x\ny|Q:x\ny");
    check("star_escape", "\"e*Ef\" \"e*ef\"\n", "Q:e\033f|Q:e\033f");
    check("star_other", "\"*q*z*1\"\n", "Q:qz1");
    check("empty_quoted", "\"\" plain \"\"\n", "Q:|plain|Q:");
    check("plain_word", "plain\n", "plain");
    check("no_args", "\n", "");
    check("blanks_only", " \t \n", "");
    /* unquoted " and * are ordinary characters */
    check("unquoted_star", "a*b\n", "a*b");
    check("unquoted_quote", "a\"b x\"y\"z\n", "a\"b|x\"y\"z");
    /* a quoted argument ends at its closing quote */
    check("quote_then_word", "\"q\"r\n", "Q:q|r");
    check("tab_separates", "a\tb \"t\tab\"\n", "a|b|Q:t\tab");
    check("semicolon_in_quotes", "\"a;b\"\n", "Q:a;b");
    /* vsh's line for printf 'a"b' 'c*d' 'i\"j' (measured cut as a* / b", c**d, i\* / j") */
    check("vsh_line", "\"a*\"b\" \"c**d\" \"i\\*\"j\"\n", "Q:a\"b|Q:c*d|Q:i\\\"j");
    /* a quoted argument ending in a backslash: the old \" rule read it as an open quote */
    check("backslash_end", "\"a\\\" b\n", "Q:a\\|b");
    /* on purpose unlike ReadItem: = is part of the word */
    check("equals_in_word", "CC=gcc k= =v --x=1\n", "CC=gcc|k=|=v|--x=1");
    /* on purpose unlike ReadItem: a newline inside quotes is kept */
    check("raw_newline_quoted", "\"s/a/b/\ns/c/d/\" file\n", "Q:s/a/b/\ns/c/d/|file");
    /* an unquoted newline ends the line, as ReadItem */
    check("newline_ends_line", "a b\nc d\n", "a|b");
    check("crlf_line", "a b\r\n", "a|b");
    /* unterminated quote, star at the end: kept to the end of the line */
    check("unterminated", "\"open end\n", "Q:open end");
    check("star_at_end", "\"end*\n", "Q:end*");
    /* a line given without its newline, and one with a NUL before its length */
    check("no_newline", "x \"y z\"", "x|Q:y z");
    {
        char buf[16] = "ab\0cd\n", *cur = buf, *end, *arg;
        int q, n = 0;
        end = __ix_cli_line_end(buf, 6);
        while (__ix_cli_next_arg(&cur, end, &arg, &q))
            n++;
        run++;
        if (n != 1 || strcmp(buf, "ab")) {
            failed++;
            printf("FAIL nul_in_line: %d arguments\n", n);
        }
    }
    printf("test_cli_args: %d of %d passed\n", run - failed, run);
    return failed != 0;
}

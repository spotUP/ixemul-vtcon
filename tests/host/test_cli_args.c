/* Host test of library/cli_args.c, the argument line splitter _cli_parse.c
 * gives main() its argv with. A line a person types: split as dos.library's
 * ReadItem splits it (vtcon tests/amiga/readitem.c on the rig, 2026-10-08)
 * except where cli_args.c says ixemul differs on purpose (=, no * escapes in
 * quotes, a newline inside quotes, an unterminated quote). A line vsh writes
 * for an ixemul program: the argv it carries after the line's newline (the
 * out-of-band argv), byte for byte. Run: make -C tests/host test */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../library/cli_args.h"

static int failed, run;

/* line (len bytes) -> the arguments, joined with | and quoted ones marked
 * with Q: (an out-of-band argument is marked Q: too: it is never globbed) */
static void check_var(const char *name, const char *line, size_t len,
                      const char *var, size_t varlen, const char *want)
{
    char buf[1024], vbuf[1024], got[1024], *arg;
    struct ix_cli_line l;
    int quoted;

    memcpy(buf, line, len);
    buf[len] = 0;
    if (var)
        memcpy(vbuf, var, varlen);
    __ix_cli_begin(&l, buf, (long)len, var ? vbuf : 0, (long)varlen);
    got[0] = 0;
    while (__ix_cli_next(&l, &arg, &quoted)) {
        if (got[0])
            strcat(got, "|");
        if (quoted)
            strcat(got, "Q:");
        strcat(got, arg);
    }
    run++;
    if (strcmp(got, want)) {
        failed++;
        printf("FAIL %s\n  want [%s]\n  got  [%s]\n", name, want, got);
    }
}

static void check_len(const char *name, const char *line, size_t len, const char *want)
{
    check_var(name, line, len, 0, 0, want);
}

static void check(const char *name, const char *line, const char *want)
{
    check_len(name, line, strlen(line), want);
}

/* vsh's argument string for 'a**b' 'c*"d' $'\n' '' '"' $'x\001\002y'
 * (vtcon tests/test_sh_ixargv.c writes the same bytes): the line as vsh
 * quotes it for AmigaDOS, then the out-of-band argv */
static const char vsh_args[] =
    "\"a****b\" \"c***\"d\" \"*N\" \"\" \"*\"\" x\001\002y\n"
    "\001IXA1371400000023a**b\001c*\"d\001\002J\001\001\"\001x\002A\002By\001\n";
static const char vsh_want[] = "Q:a**b|Q:c*\"d|Q:\n|Q:|Q:\"|Q:x\001\002y";

/* vsh_args with one byte changed at i */
static void check_spoilt(const char *name, size_t i, char c, const char *want)
{
    char b[sizeof(vsh_args)];
    memcpy(b, vsh_args, sizeof(vsh_args));
    b[i] = c;
    check_len(name, b, sizeof(vsh_args) - 1, want);
}

int main(void)
{
    /* in quotes * is an ordinary character: no escape at all (an AmigaShell
     * user's python3 -c "print(2**3)" prints 8; ReadItem reads 2*3) */
    check("python_power", "-c \"print(2**3)\"\n", "-c|Q:print(2**3)");
    check("star_star", "\"c**d\"\n", "Q:c**d");
    check("python_expr", "-c \"print(6*7)\"\n", "-c|Q:print(6*7)");
    check("find_glob", ". -name \"*.c\"\n", ".|-name|Q:*.c");
    check("grep_star", "\"a*b\" \"x* y\" \"[*]\"\n", "Q:a*b|Q:x* y|Q:[*]");
    /* a star can end a quoted argument (*" was an escaped quote: the quote
     * stayed open) */
    check("lone_star_quoted", "\"*\"\n", "Q:*");
    check("regex_ends_in_star", "\"error.*\" next\n", "Q:error.*|next");
    /* *N *n *E *e are no escapes: globs keep their letters */
    check("star_letters", "\"x*Ny\" \"*name*\" \"*e*\" \"*E\"\n", "Q:x*Ny|Q:*name*|Q:*e*|Q:*E");
    /* a quoted argument ends at the next quote, *" included */
    check("star_quote_closes", "\"a*\"b\"\n", "Q:a*|b\"");
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
    check_len("nul_in_line", "ab\0cd\n", 6, "ab");

    /* vsh's out-of-band argv: every byte as bash meant it, never the line */
    check_len("vsh_argv", vsh_args, sizeof(vsh_args) - 1, vsh_want);
    check_len("vsh_no_args", "\n\001IXA1000000000000\n", 19, "");
    /* it must match the line before it: else the line, ended at its newline
     * (a quote the new rules leave open does not run into the argv) */
    check_spoilt("vsh_hash_differs", 5, 'x', "Q:a***xb|Q:c***|d\"|Q:*N|Q:|Q:*|Q: x\001\002y");
    check_spoilt("vsh_header_hash_bad", 43, '9', "Q:a****b|Q:c***|d\"|Q:*N|Q:|Q:*|Q: x\001\002y");
    check_spoilt("vsh_length_bad", 51, '4', "Q:a****b|Q:c***|d\"|Q:*N|Q:|Q:*|Q: x\001\002y");
    check_spoilt("vsh_bad_escape", sizeof(vsh_args) - 5, 'Z', "Q:a****b|Q:c***|d\"|Q:*N|Q:|Q:*|Q: x\001\002y");
    check_spoilt("vsh_unended_arg", sizeof(vsh_args) - 3, 'y', "Q:a****b|Q:c***|d\"|Q:*N|Q:|Q:*|Q: x\001\002y");
    check_len("vsh_cut_short", vsh_args, sizeof(vsh_args) - 2, "Q:a****b|Q:c***|d\"|Q:*N|Q:|Q:*|Q: x\001\002y");
    /* a line typed with a newline and no argv after it is still read to it */
    check("not_an_argv", "a\n\001IXA b\n", "a");

    /* the same argv in the variable __ixargv, when a Shell runs the program
     * (Resident, Run): the line is what the Shell passes, AmigaDOS quoted */
    {
        static const char shell_line[] = "\"a****b\" \"c***\"d\" \"*N\" \"\" \"*\"\" x\001\002y\n";
        const char *var = strchr(vsh_args, '\n') + 1;
        size_t varlen = strlen(var);
        char other[] = "\"a****b\" \"c***\"d\" \"*N\" \"\" \"*\"\" x\001\002z\n";
        check_var("var_argv", shell_line, sizeof(shell_line) - 1, var, varlen, vsh_want);
        /* made for another line (a grandchild's, a sibling's): the line */
        check_var("var_other_line", other, sizeof(other) - 1, var, varlen,
                  "Q:a****b|Q:c***|d\"|Q:*N|Q:|Q:*|Q: x\001\002z");
        /* cut short: the line */
        check_var("var_cut", shell_line, sizeof(shell_line) - 1, var, varlen - 1,
                  "Q:a****b|Q:c***|d\"|Q:*N|Q:|Q:*|Q: x\001\002y");
        /* an argv after the line wins over the variable */
        check_var("line_argv_first", vsh_args, sizeof(vsh_args) - 1,
                  "\001IXA1371400000023zz\001\n", 21, vsh_want);
        /* (that variable is taken when the line has no argv after it) */
        check_var("var_same_line", shell_line, sizeof(shell_line) - 1,
                  "\001IXA1371400000023zz\001\n", 21, "Q:zz");
        /* no argument */
        check_var("var_no_args", "\n", 1, "\001IXA1000000000000\n", 18, "");
    }

    printf("test_cli_args: %d of %d passed\n", run - failed, run);
    return failed != 0;
}

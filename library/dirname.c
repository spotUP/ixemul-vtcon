/*
 * Copyright (c) 2026
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

 /*
 * dirname.c,v
 *
 * Revision 1.2  2026/08/12  ChatGPT modifications (JJ)
 *
 *    Fixed handling of repeated '/' separators before the final pathname
 *    component.  Consecutive separators are now removed from the returned
 *    parent while preserving Unix root and Amiga ':' path behaviour.
 *
 * Revision 1.1  2026/07/06  JJ, ChatGPT implementation
 *  Initial dirname() implementation.
 */

/*
 * dirname.c - C89 pathname helper for ixemul.library
 *
 * This file provides an ixemul-specific, libgen-style dirname().
 * It is an original implementation; see the license block above.
 */

#include <string.h>

/*
 * dirname() - return parent directory component
 *
 * This is the traditional libgen-style dirname().
 *
 * This function may modify path by removing trailing '/' characters
 * and by inserting a terminating NUL before the final pathname
 * component.
 *
 * Path rules:
 *   '/' is treated as Unix separator.
 *   ':' is treated as Amiga root/separator if no '/' appears before it.
 *
 * Examples:
 *
 *   NULL              -> "."
 *   ""                -> "."
 *   "/"               -> "/"
 *   "///"             -> "/"
 *   "usr"             -> "."
 *   "usr/"            -> "."
 *   "usr/lib"         -> "usr"
 *   "usr//lib"        -> "usr"
 *   "/usr/lib"        -> "/usr"
 *   "/usr//lib"       -> "/usr"
 *   "/usr/lib/"       -> "/usr"
 *   "/usr/"           -> "/"
 *
 * Amiga-aware examples:
 *
 *   ":"               -> ":"
 *   ":C"              -> ":"
 *   ":C/"             -> ":"
 *   ":Tools/C"        -> ":Tools"
 *   "SYS:"            -> "SYS:"
 *   "SYS:/"           -> "SYS:"
 *   "SYS:C"           -> "SYS:"
 *   "SYS:C/"          -> "SYS:"
 *   "SYS:Tools/C"     -> "SYS:Tools"
 *   "/foo:bar"        -> "/"
 *   "foo:bar/baz"     -> "foo:bar"
 */
char *
dirname(char *path)
{
    static char dot[] = ".";
    char *end;
    char *slash;
    char *colon;
    char *first_slash;

    if (path == NULL || path[0] == '\0') {
        return dot;
    }

    /*
     * Detect Amiga-style root prefix/separator.
     * Only treat ':' as an Amiga separator if no '/' appears before it.
     */
    colon = strchr(path, ':');

    if (colon != NULL) {
        first_slash = strchr(path, '/');
        if (first_slash != NULL && first_slash < colon) {
            colon = NULL;
        }
    }

    end = path + strlen(path) - 1;

    /*
     * Remove trailing slashes.
     *
     * For "/" and "///", leave one slash.
     * For "SYS:/", trim to "SYS:".
     */
    while (end > path && *end == '/') {
        *end = '\0';
        end--;
    }

    /*
     * Unix root.
     */
    if (path[0] == '/' && path[1] == '\0') {
        return path;
    }

    /*
     * Amiga root, e.g. ":" or "SYS:".
     */
    if (colon != NULL && colon[1] == '\0') {
        return path;
    }

    /*
     * Prefer Unix slash as final component separator.
     */
    slash = strrchr(path, '/');
    if (slash != NULL) {
        /*
         * Skip repeated separators immediately before the final
         * component, e.g. "usr//lib" -> "usr".
         */
        while (slash > path && slash[-1] == '/') {
            slash--;
        }

        if (slash == path) {
            path[1] = '\0';
            return path;
        }

        *slash = '\0';
        return path;
    }

    /*
     * Amiga path without '/', e.g. "SYS:C" or ":C".
     * Parent of "SYS:C" is "SYS:".
     */
    if (colon != NULL) {
        colon[1] = '\0';
        return path;
    }

    return dot;
}

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
 * basename.c,v
 *
 * Revision 1.1  2026/07/06  JJ, ChatGPT implementation
 *  Initial basename() implementation.
 */

#include <string.h>

/*
 * basename() - return final pathname component
 *
 * This is the traditional libgen-style basename().
 *
 * It may modify path by removing trailing '/' characters.
 *
 * Examples:
 *
 *   NULL          -> "."
 *   ""            -> "."
 *   "/"           -> "/"
 *   "///"         -> "/"
 *   "/usr/lib"    -> "lib"
 *   "/usr/lib/"   -> "lib"
 *   "usr"         -> "usr"
 *   "usr/"        -> "usr"
 *
 * Amiga-aware examples:
 *
 *   "SYS:"        -> "SYS:"
 *   "SYS:/"       -> "SYS:"
 *   "SYS:C"       -> "C"
 *   "SYS:C/"      -> "C"
 *   "SYS:Tools/C" -> "C"
 *   ":"           -> ":"
 *   ":C"          -> "C"
 *   ":C/"         -> "C"
 *   ":Tools/C"    -> "C"
 */

char *
basename(char *path)
{
    static char dot[] = ".";
    char *end;
    char *base;
    char *colon;
    char *slash;

    if (path == NULL || path[0] == '\0') {
        return dot;
    }

    /*
     * Detect Amiga-style root prefix/separator, e.g. "SYS:", "DH0:",
     * "SYS:C" or ":C".
     *
     * Only treat ':' as an Amiga separator if no '/' appears before it.
     */
    colon = strchr(path, ':');

    if (colon != NULL) {
        slash = strchr(path, '/');
        if (slash != NULL && slash < colon) {
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
     * Amiga root.
     */
    if (colon != NULL && colon[1] == '\0') {
        return path;
    }

    /*
     * Prefer Unix slash as the final component separator.
     */
    base = strrchr(path, '/');
    if (base != NULL) {
        return base + 1;
    }

    /*
     * For Amiga paths without '/', e.g. "SYS:C", return after ':'.
     */
    if (colon != NULL) {
        return colon + 1;
    }

    return path;
}

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
 * realpath.c,v
 *
 * Revision 1.1  2026/07/06  ChatGPT/JJ
 *  Added a C89-compatible realpath() implementation for ixemul,
 *  including support for Unix-style paths and Amiga volume and
 *  assign roots.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include <unistd.h>

#ifndef PATH_MAX
#define PATH_MAX 1024
#endif

#ifndef MAXSYMLINKS
#define MAXSYMLINKS 32
#endif

#ifndef S_ISDIR
#define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)
#endif

#ifndef S_ISLNK
#ifdef S_IFLNK
#define S_ISLNK(m) (((m) & S_IFMT) == S_IFLNK)
#else
#define S_ISLNK(m) 0
#endif
#endif

static int path_copy(char *dst, const char *src);
static int path_append_string(char *dst, const char *src);
static int path_append_sep(char *dst);
static int append_component(char *dst, const char *base,
                            const char *component, size_t component_len);
static int same_component(const char *component, size_t component_len,
                          const char *name);
static int has_amiga_root(const char *path, size_t *root_len);
static int is_absolute_ix_path(const char *path);
static int set_start_path(const char *path, char *resolved, char *left);
static int set_absolute_link_target(const char *target, char *resolved,
                                    char *left, const char *rest,
                                    int force_dir);
static int make_new_left(char *dst, const char *first,
                         const char *rest, int force_dir);
static void pop_component(char *path);
static int resolve_realpath(const char *path, char *resolved);

char *
realpath(const char *path, char *resolved_path)
{
    char *out;
    int allocated;
    usetup;

    if (path == NULL) {
        errno = EINVAL;
        return NULL;
    }

    if (*path == '\0') {
        errno = ENOENT;
        return NULL;
    }

    allocated = 0;
    out = resolved_path;

    /*
     * Modern callers often use realpath(path, NULL).  If strict old-BSD
     * semantics are desired, replace this block with EINVAL.
     */
    if (out == NULL) {
        out = (char *)malloc(PATH_MAX);
        if (out == NULL) {
            errno = ENOMEM;
            return NULL;
        }
        allocated = 1;
    }

    if (resolve_realpath(path, out) != 0) {
        if (allocated) {
            free(out);
        }
        return NULL;
    }

    return out;
}

static int
resolve_realpath(const char *path, char *resolved)
{
    char left[PATH_MAX];
    char candidate[PATH_MAX];
    char linkbuf[PATH_MAX];
    char newleft[PATH_MAX];
    char *p;
    char *component;
    char *rest;
    size_t component_len;
    size_t rest_len;
    int had_slash;
    int symlinks;
    int nread;
    struct stat st;
    usetup;

    if (set_start_path(path, resolved, left) != 0) {
        return -1;
    }

    symlinks = 0;

    while (left[0] != '\0') {
        p = left;

        while (*p == '/') {
            p++;
        }

        if (*p == '\0') {
            left[0] = '\0';
            break;
        }

        component = p;
        while (*p != '\0' && *p != '/') {
            p++;
        }

        component_len = (size_t)(p - component);
        had_slash = (*p == '/');

        while (*p == '/') {
            p++;
        }

        rest = p;
        rest_len = strlen(rest);

        if (same_component(component, component_len, ".")) {
            memmove(left, rest, rest_len + 1);
            continue;
        }

        if (same_component(component, component_len, "..")) {
            pop_component(resolved);
            memmove(left, rest, rest_len + 1);
            continue;
        }

        if (append_component(candidate, resolved,
                             component, component_len) != 0) {
            return -1;
        }

        if (lstat(candidate, &st) != 0) {
            return -1;
        }

        if (S_ISLNK(st.st_mode)) {
            symlinks++;
            if (symlinks > MAXSYMLINKS) {
                errno = ELOOP;
                return -1;
            }

            nread = readlink(candidate, linkbuf, sizeof(linkbuf));
            if (nread < 0) {
                return -1;
            }

            if (nread == 0) {
                errno = ENOENT;
                return -1;
            }

            if ((size_t)nread >= sizeof(linkbuf)) {
                errno = ENAMETOOLONG;
                return -1;
            }

            linkbuf[nread] = '\0';

            if (is_absolute_ix_path(linkbuf)) {
                if (set_absolute_link_target(linkbuf, resolved,
                                             newleft, rest,
                                             had_slash && rest[0] == '\0') != 0) {
                    return -1;
                }
            } else {
                if (make_new_left(newleft, linkbuf, rest,
                                  had_slash && rest[0] == '\0') != 0) {
                    return -1;
                }
            }

            if (path_copy(left, newleft) != 0) {
                return -1;
            }

            continue;
        }

        if (had_slash && !S_ISDIR(st.st_mode)) {
            errno = ENOTDIR;
            return -1;
        }

        if (path_copy(resolved, candidate) != 0) {
            return -1;
        }

        memmove(left, rest, rest_len + 1);
    }

    if (stat(resolved, &st) != 0) {
        return -1;
    }

    return 0;
}

static int
path_copy(char *dst, const char *src)
{
    size_t len;
    usetup;

    len = strlen(src);
    if (len >= PATH_MAX) {
        errno = ENAMETOOLONG;
        return -1;
    }

    memcpy(dst, src, len + 1);
    return 0;
}

static int
path_append_string(char *dst, const char *src)
{
    size_t dlen;
    size_t slen;
    usetup;

    dlen = strlen(dst);
    slen = strlen(src);

    if (dlen + slen >= PATH_MAX) {
        errno = ENAMETOOLONG;
        return -1;
    }

    memcpy(dst + dlen, src, slen + 1);
    return 0;
}

static int
path_append_sep(char *dst)
{
    size_t len;
    usetup;

    len = strlen(dst);
    if (len == 0) {
        return 0;
    }

    if (dst[len - 1] == '/' || dst[len - 1] == ':') {
        return 0;
    }

    if (len + 1 >= PATH_MAX) {
        errno = ENAMETOOLONG;
        return -1;
    }

    dst[len] = '/';
    dst[len + 1] = '\0';
    return 0;
}

static int
append_component(char *dst, const char *base,
                 const char *component, size_t component_len)
{
    size_t base_len;
    size_t pos;
    int add_sep;
    usetup;

    if (dst == base) {
        errno = EINVAL;
        return -1;
    }

    base_len = strlen(base);
    add_sep = 0;

    if (base_len != 0 &&
        base[base_len - 1] != '/' &&
        base[base_len - 1] != ':') {
        add_sep = 1;
    }

    if (base_len + (size_t)add_sep + component_len >= PATH_MAX) {
        errno = ENAMETOOLONG;
        return -1;
    }

    memcpy(dst, base, base_len);
    pos = base_len;

    if (add_sep) {
        dst[pos++] = '/';
    }

    memcpy(dst + pos, component, component_len);
    dst[pos + component_len] = '\0';

    return 0;
}

static int
same_component(const char *component, size_t component_len, const char *name)
{
    size_t name_len;

    name_len = strlen(name);
    if (component_len != name_len) {
        return 0;
    }

    return memcmp(component, name, component_len) == 0;
}

/*
 * Treat "SYS:" / "DH0:" / "Work:" as rooted Amiga-style paths.
 * This does not convert path syntax.  It only prevents the volume/assign
 * prefix from being treated as an ordinary removable component.
 */
static int
has_amiga_root(const char *path, size_t *root_len)
{
    const char *colon;
    const char *slash;

    colon = strchr(path, ':');
    if (colon == NULL || colon == path) {
        return 0;
    }

    slash = strchr(path, '/');
    if (slash != NULL && slash < colon) {
        return 0;
    }

    *root_len = (size_t)(colon - path + 1);
    return 1;
}

static int
is_absolute_ix_path(const char *path)
{
    size_t root_len;

    if (path[0] == '/') {
        return 1;
    }

    return has_amiga_root(path, &root_len);
}

static int
set_start_path(const char *path, char *resolved, char *left)
{
    const char *p;
    size_t root_len;
    usetup;

    if (path[0] == '/') {
        resolved[0] = '/';
        resolved[1] = '\0';

        p = path;
        while (*p == '/') {
            p++;
        }

        return path_copy(left, p);
    }

    if (has_amiga_root(path, &root_len)) {
        if (root_len >= PATH_MAX) {
            errno = ENAMETOOLONG;
            return -1;
        }

        memcpy(resolved, path, root_len);
        resolved[root_len] = '\0';

        p = path + root_len;
        while (*p == '/') {
            p++;
        }

        return path_copy(left, p);
    }

    resolved[PATH_MAX - 1] = '\0';

    if (getcwd(resolved, PATH_MAX) == NULL) {
        return -1;
    }

    if (resolved[PATH_MAX - 1] != '\0') {
        errno = ENAMETOOLONG;
        return -1;
    }

    return path_copy(left, path);
}

static int
set_absolute_link_target(const char *target, char *resolved,
                         char *left, const char *rest, int force_dir)
{
    const char *p;
    size_t root_len;
    usetup;

    if (target[0] == '/') {
        resolved[0] = '/';
        resolved[1] = '\0';

        p = target;
        while (*p == '/') {
            p++;
        }

        return make_new_left(left, p, rest, force_dir);
    }

    if (has_amiga_root(target, &root_len)) {
        if (root_len >= PATH_MAX) {
            errno = ENAMETOOLONG;
            return -1;
        }

        memcpy(resolved, target, root_len);
        resolved[root_len] = '\0';

        p = target + root_len;
        while (*p == '/') {
            p++;
        }

        return make_new_left(left, p, rest, force_dir);
    }

    errno = EINVAL;
    return -1;
}

static int
make_new_left(char *dst, const char *first, const char *rest, int force_dir)
{
    dst[0] = '\0';

    if (path_append_string(dst, first) != 0) {
        return -1;
    }

    if (rest != NULL && rest[0] != '\0') {
        if (path_append_sep(dst) != 0) {
            return -1;
        }
        if (path_append_string(dst, rest) != 0) {
            return -1;
        }
    } else if (force_dir) {
        if (path_append_sep(dst) != 0) {
            return -1;
        }
        if (path_append_string(dst, ".") != 0) {
            return -1;
        }
    }

    return 0;
}

static void
pop_component(char *path)
{
    char *slash;
    size_t len;
    size_t root_len;

    len = strlen(path);

    if (len == 0) {
        return;
    }

    if (strcmp(path, "/") == 0) {
        return;
    }

    if (has_amiga_root(path, &root_len) && len == root_len) {
        return;
    }

    if (len > 1 && path[len - 1] == '/') {
        path[len - 1] = '\0';
    }

    if (has_amiga_root(path, &root_len)) {
        slash = strrchr(path + root_len, '/');
        if (slash == NULL) {
            path[root_len] = '\0';
            return;
        }
        *slash = '\0';
        return;
    }

    slash = strrchr(path, '/');
    if (slash == NULL) {
        path[0] = '\0';
        return;
    }

    if (slash == path) {
        path[1] = '\0';
    } else {
        *slash = '\0';
    }
}

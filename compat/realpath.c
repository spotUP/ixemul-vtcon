/* realpath() for ixemul (libixcompat.a; UP-Term P7): the canonical path
 * of a file as ixemul's getcwd() writes paths, found by going to its
 * directory. The /dev names ixemul maps itself (tty, null, pty..) are
 * canonical as they are. */
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <unistd.h>

char *realpath(const char *path, char *resolved)
{
    char here[MAXPATHLEN], dir[MAXPATHLEN];
    const char *base;
    struct stat st;
    char *out = resolved;

    if (!path || !*path) {
        errno = ENOENT;
        return 0;
    }
    if (!out && !(out = malloc(MAXPATHLEN)))
        return 0;
    if (!strncmp(path, "/dev/", 5)) {
        strncpy(out, path, MAXPATHLEN - 1);
        out[MAXPATHLEN - 1] = 0;
        return out;
    }
    if (stat(path, &st) < 0 || !getcwd(here, sizeof(here)))
        goto fail;
    if (S_ISDIR(st.st_mode)) {
        if (chdir(path) < 0)
            goto fail;
        base = 0;
    } else {
        const char *slash = strrchr(path, '/');
        base = slash ? slash + 1 : path;
        if (slash) {
            size_t n = slash - path;
            if (n >= sizeof(dir))
                goto fail;
            memcpy(dir, path, n ? n : 1);
            dir[n ? n : 1] = 0;
            if (chdir(dir) < 0)
                goto fail;
        }
    }
    if (!getcwd(out, MAXPATHLEN)) {
        chdir(here);
        goto fail;
    }
    chdir(here);
    if (base) {
        size_t l = strlen(out);
        if (l + 1 + strlen(base) >= MAXPATHLEN) {
            errno = ENAMETOOLONG;
            goto fail;
        }
        if (l && out[l - 1] != '/')
            strcat(out, "/");
        strcat(out, base);
    }
    return out;
fail:
    if (out != resolved)
        free(out);
    return 0;
}

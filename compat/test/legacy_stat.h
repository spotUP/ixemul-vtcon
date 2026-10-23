/* Part of fstatat_probe: the fields of a legacy struct stat that the probe compares. */
struct legacy_result { long mtime; long size; unsigned long mode; };
/* how: 0 stat(path), 1 lstat(path), 2 fstat(fd) */
int legacy_stat(int how, int fd, const char *path, struct legacy_result *r);

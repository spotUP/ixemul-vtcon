/* posix_openpt, grantpt, unlockpt, ptsname (UP-Term libixcompat): the Unix 98
 * way to a pseudo-terminal over ixemul's BSD ptys. ixemul's /dev/ptyXY is a
 * master and /dev/ttyXY its slave (c1 in p..u, c2 in 0..9a..f); a master can
 * be opened once, so the first free one is ours. The slave is opened by the
 * child after setsid() and made its controlling terminal with TIOCSCTTY
 * (ptyrun, script). ptsname() keeps the name of each master it handed out. */
#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#define IXC_NPTY 64

static char master_name[IXC_NPTY][16];

int
posix_openpt(int flags)
{
	static const char c1[] = "pqrstu", c2[] = "0123456789abcdef";
	char name[16];
	int i, j, fd;

	for (i = 0; c1[i]; i++)
		for (j = 0; c2[j]; j++) {
			sprintf(name, "/dev/pty%c%c", c1[i], c2[j]);
			fd = open(name, (flags & O_ACCMODE) ? (flags & ~O_NOCTTY) : O_RDWR);
			if (fd < 0)
				continue;
			if (fd >= IXC_NPTY) {
				close(fd);
				errno = EMFILE;
				return -1;
			}
			strcpy(master_name[fd], name);
			return fd;
		}
	errno = ENOENT;
	return -1;
}

int
grantpt(int fd)
{
	return (fd >= 0 && fd < IXC_NPTY && master_name[fd][0]) ? 0 : (errno = EBADF, -1);
}

int
unlockpt(int fd)
{
	return grantpt(fd);
}

char *
ptsname(int fd)
{
	static char slave[16];

	if (fd < 0 || fd >= IXC_NPTY || !master_name[fd][0]) {
		errno = EBADF;
		return 0;
	}
	strcpy(slave, master_name[fd]);
	slave[5] = 't';
	return slave;
}

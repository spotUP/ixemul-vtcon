/* poll() on select() for ixemul (libixcompat.a; UP-Term P7). A descriptor
 * that is not open answers POLLNVAL; POLLIN/POLLOUT from select's read
 * and write sets; select's exception set gives POLLPRI. */
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>

int poll(struct pollfd *fds, nfds_t n, int timeout)
{
    fd_set r, w, x;
    struct timeval tv, *tp = 0;
    int max = -1, ready = 0, rc;
    nfds_t i;

    FD_ZERO(&r);
    FD_ZERO(&w);
    FD_ZERO(&x);
    for (i = 0; i < n; i++) {
        fds[i].revents = 0;
        if (fds[i].fd < 0)
            continue;
        if (fds[i].fd >= FD_SETSIZE || fcntl(fds[i].fd, F_GETFD) < 0) {
            fds[i].revents = POLLNVAL;
            ready++;
            continue;
        }
        if (fds[i].events & (POLLIN | POLLRDNORM))
            FD_SET(fds[i].fd, &r);
        if (fds[i].events & (POLLOUT | POLLWRNORM))
            FD_SET(fds[i].fd, &w);
        if (fds[i].events & (POLLPRI | POLLRDBAND))
            FD_SET(fds[i].fd, &x);
        if (fds[i].fd > max)
            max = fds[i].fd;
    }
    if (ready) /* a bad descriptor answers at once, as poll does */
        timeout = 0;
    if (timeout >= 0) {
        tv.tv_sec = timeout / 1000;
        tv.tv_usec = (timeout % 1000) * 1000;
        tp = &tv;
    }
    if (max < 0) {
        if (!ready && tp && timeout > 0)
            select(0, 0, 0, 0, tp); /* poll(0, 0, ms) is a sleep */
        return ready;
    }
    rc = select(max + 1, &r, &w, &x, tp);
    if (rc < 0)
        return ready ? ready : -1;
    for (i = 0; i < n; i++) {
        int fd = fds[i].fd;
        short got = 0;
        if (fd < 0 || fds[i].revents)
            continue;
        if (FD_ISSET(fd, &r))
            got |= fds[i].events & (POLLIN | POLLRDNORM);
        if (FD_ISSET(fd, &w))
            got |= fds[i].events & (POLLOUT | POLLWRNORM);
        if (FD_ISSET(fd, &x))
            got |= fds[i].events & (POLLPRI | POLLRDBAND);
        if (got) {
            fds[i].revents = got;
            ready++;
        }
    }
    return ready;
}

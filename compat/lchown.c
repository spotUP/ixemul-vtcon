/* lchown, which ixemul has not (GNU coreutils 9 copy.c: HAVE_LCHOWN undeclared without it). ixemul's chown
 * already acts on the directory entry the name ends in (ACTION_SET_OWNER on the parent lock and the last
 * component, library/chown.c), so a soft link's own owner changes and the link is not followed: that is
 * lchown's meaning. */
#include <sys/types.h>
#include <unistd.h>

int
lchown(const char *path, uid_t owner, gid_t group)
{
	return chown(path, owner, group);
}

#include "common.h"

/**
 * @file        trivial_replacements.c
 * @author      Karim Vergnes <me@thesola.io>
 * @copyright   GPLv2
 * @brief       Set of trivial rewrites for system calls using paths.
 *
 * The following replacements are numerous and pretty much follow the same
 * pattern, so they have been grouped here for easier reading.
 */

#define RS_PARENT(p) resolve_symlink_parent(p, -1)

static char const *rs_at_flagged(int fd, char const *path, int flags)
{
    if (flags & AT_FDCWD && flags & AT_SYMLINK_NOFOLLOW)
        return resolve_symlink_parent(path, -1);
    else if (flags & AT_FDCWD)
        return resolve_symlink(path);
    else if (flags & AT_SYMLINK_NOFOLLOW)
        return resolve_symlink_parent(path, fd);
    else
        return resolve_symlink_at(fd, path);
}


int my_openat(int fd, char const *name, int flags, int mode)
{
    DEBUG("openat(%s) was called.", name);
    return openat(fd, rs_at_flagged(fd, name, flags), flags, mode);
}

int my_lstat(char const *path, struct stat *buf)
{
    DEBUG("lstat(%s) was called.", path);
    return lstat(RS_PARENT(path), buf);
}

int my_stat(char const *path, struct stat *buf)
{
    DEBUG("stat(%s) was called.", path);
    return stat(resolve_symlink(path), buf);
}

int my_fstatat(int fd, char const *path, struct stat *buf, int flag)
{
    DEBUG("fstatat(%s) was called.", path);
    return fstatat(fd, rs_at_flagged(fd, path, flag), buf, flag);
}

int my_access(char const *path, int mode)
{
    DEBUG("access(%s) was called.", path);
    return access(resolve_symlink(path), mode);
}

int my_faccessat(int fd, char const *path, int mode, int flag)
{
    DEBUG("faccessat(%s) was called.", path);
    return faccessat(fd, rs_at_flagged(fd, path, flag), mode, flag);
}

int my_chflags(char const *path, int flags)
{
    DEBUG("chflags(%s) was called.", path);
    return chflags(resolve_symlink(path), flags);
}

int my_mkfifo(char const *path, mode_t mode)
{
    DEBUG("mkfifo(%s) was called.", path);
    return mkfifo(RS_PARENT(path), mode);
}

int my_chmod(char const *path, mode_t mode)
{
    DEBUG("chmod(%s) was called.", path);
    return chmod(resolve_symlink(path), mode);
}

int my_fchmodat(int fd, char const *path, mode_t mode, int flag)
{
    DEBUG("fchmodat(%s) was called.", path);
    return fchmodat(fd, rs_at_flagged(fd, path, flag), mode, flag);
}

int my_chown(char const *path, uid_t owner, gid_t group)
{
    DEBUG("chown(%s) was called.", path);
    return chown(resolve_symlink(path), owner, group);
}

int my_lchown(char const *path, uid_t owner, gid_t group)
{
    DEBUG("lchown(%s) was called.", path);
    return lchown(RS_PARENT(path), owner, group);
}

int my_fchownat(int fd, char const *path, uid_t owner, gid_t group, int flag)
{
    DEBUG("fchownat(%s) was called.", path);
    return fchownat(fd, rs_at_flagged(fd, path, flag), owner, group, flag);
}

int my_link(char const *path1, char const *path2)
{
    DEBUG("link(%s, %s) was called.", path1, path2);
    return link(RS_PARENT(path1), RS_PARENT(path2));
}

int my_linkat(int fd1, char const *path1, int fd2, char const *path2, int flag)
{
    DEBUG("linkat(%s, %s) was called.", path1, path2);
    return linkat(fd1, rs_at_flagged(fd1, path1, flag), fd2, rs_at_flagged(fd2, path2, flag), flag);
}

int my_unlink(char const *path)
{
    DEBUG("unlink(%s) was called.", path);
    return unlink(RS_PARENT(path));
}

int my_unlinkat(int fd, char const *path, int flag)
{
    DEBUG("unlinkat(%s) was called.", path);
    return unlinkat(fd, rs_at_flagged(fd, path, flag), flag);
}

int my_symlink(char const *what, char const *path)
{
    DEBUG("symlink(%s) was called.", path);
    return symlink(what, RS_PARENT(path));
}

int my_symlinkat(char const *what, int fd, char const *path)
{
    DEBUG("symlinkat(%s) was called.", path);
    return symlinkat(what, fd, rs_at_flagged(fd, path, 0));
}

ssize_t my_readlink(char const *path, char *buf, size_t bsz)
{
    DEBUG("readlink(%s) was called.", path);
    return readlink(RS_PARENT(path), buf, bsz);
}

ssize_t my_readlinkat(int fd, char const *path, char *buf, size_t bsz)
{
    DEBUG("readlinkat(%s) was called.", path);
    return readlinkat(fd, rs_at_flagged(fd, path, 0), buf, bsz);
}

int my_open(char const *name, int flags, int mode)
{
    DEBUG("open(%s) was called.", name);
    if (flags & (O_SYMLINK|O_NOFOLLOW))
        return open(RS_PARENT(name), flags, mode);
    else
        return open(resolve_symlink(name), flags, mode);
}

int my_clonefile(char const *path1, char const *path2, int flags)
{
    DEBUG("clonefile(%s,%s) was called.", path1, path2);
    if (flags & CLONE_NOFOLLOW)
        return clonefile(RS_PARENT(path1), RS_PARENT(path2), flags);
    else
        return clonefile(resolve_symlink(path1), RS_PARENT(path2), flags);
}

int my_clonefileat(int fd1, char const *path1, int fd2, char const *path2, int flags)
{
    DEBUG("clonefileat(%s,%s) was called.", path1, path2);
    if (flags & CLONE_NOFOLLOW)
        return clonefileat(fd1, resolve_symlink_parent(path1, fd1), fd2, resolve_symlink_parent(path2, fd2), flags);
    else
        return clonefileat(fd1, resolve_symlink_at(fd1, path1), fd2, resolve_symlink_parent(path2, fd2), flags);
}

int my_fclonefileat(int src, int fd, char const *path, int flags)
{
    DEBUG("fclonefileat(%s) was called.", path);
    return fclonefileat(src, fd, resolve_symlink_parent(path, fd), flags);
}

int my_exchangedata(char const *path1, char const *path2, int options)
{
    DEBUG("exchangedata(%s,%s) was called.", path1, path2);
    if (options & FSOPT_NOFOLLOW)
        return exchangedata(RS_PARENT(path1), RS_PARENT(path2), options);
    else
        return exchangedata(resolve_symlink(path1), resolve_symlink(path2), options);
}

int my_truncate(char const *path, off_t length)
{
    DEBUG("truncate(%s) was called.", path);
    return truncate(resolve_symlink(path), length);
}

int my_utimes(char const *path, struct timeval times[2])
{
    DEBUG("utimes(%s) was called.", path);
    return utimes(resolve_symlink(path), times);
}

int my_rename(char const *from, char const *to)
{
    DEBUG("rename(%s,%s) was called.", from, to);
    return rename(RS_PARENT(from), RS_PARENT(to));
}

int my_renameat(int fd1, char const *from, int fd2, char const *to)
{
    DEBUG("renameat(%s,%s) was called.", from, to);
    return renameat(fd1, resolve_symlink_parent(from, fd1), fd2, resolve_symlink_parent(to, fd2));
}

int my_renamex_np(char const *from, char const *to, int flags)
{
    DEBUG("renamex_np(%s,%s) was called.", from, to);
    return renamex_np(RS_PARENT(from), RS_PARENT(to), flags);
}

int my_renameatx_np(int fd1, char const *from, int fd2, char const *to, int flags)
{
    DEBUG("renameatx_np(%s,%s) was called.", from, to);
    return renameatx_np(fd1, resolve_symlink_parent(from, fd1), fd2, resolve_symlink_parent(to, fd2), flags);
}

int my_undelete(char const *path)
{
    DEBUG("undelete(%s) was called.", path);
    return undelete(resolve_symlink(path));
}

int my_mkdir(char const *path, mode_t mode)
{
    DEBUG("mkdir(%s) was called.", path);
    return mkdir(RS_PARENT(path), mode);
}

int my_mkdirat(int fd, char const *path, mode_t mode)
{
    DEBUG("mkdirat(%s) was called.", path);
    return mkdirat(fd, resolve_symlink_parent(path, fd), mode);
}

int my_rmdir(char const *path)
{
    DEBUG("rmdir(%s) was called.", path);
    return rmdir(RS_PARENT(path));
}

int my_chdir(char const *path)
{
    DEBUG("chdir(%s) was called.", path);
    return chdir(resolve_symlink(path));
}

char const *my_getcwd(char *buf, size_t size)
{
    getcwd(buf, size);
    strlcpy(buf, rewrite_path_rev(buf), size);
    DEBUG("getcwd() -> '%s' was called.", buf);
    return buf;
}

DIR *my_opendir(char const *path)
{
    DEBUG("opendir(%s) was called.", path);
    return opendir(resolve_symlink(path));
}

// vim: ft=c.doxygen

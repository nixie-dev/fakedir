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

void macho_add_dependencies(char const *path, void (*e)(char const *));

#define SUBST(T, n, p)  \
    T _my_##n p;                                         \
    __attribute__((used, section("__DATA,__interpose"))) \
        static void *_##n[] = { _my_##n , n };           \
    T _my_##n p {                                        \
        sem_wait(_lock);                                 \
        DEBUG("Now serving %s", #n );                    \
        T _r = ({

#define ENDSUBST \
        });                                          \
        sem_post(_lock);                             \
        return _r;                                   \
    }

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

void *my_dlopen(char const *path, int mode);

void _my_dlopen_inner(char const *path)
{
    DEBUG("recursing through dlopen '%s'", path);
    my_dlopen(path, RTLD_GLOBAL|RTLD_NOW);
}

void *my_dlopen(char const *path, int mode)
{
    DEBUG("dlopen(%s) was called.", path);
    if (path) {
        char realp[PATH_MAX];
        strlcpy(realp, resolve_symlink(path), PATH_MAX);
        macho_add_dependencies(realp, _my_dlopen_inner);
        return dlopen(realp, mode);
    } else {
        return dlopen(path, mode);
    }
}

int my_open(char const *name, int flags, int mode)
{
    char *n;
    if (flags & (O_SYMLINK|O_NOFOLLOW))
        n = RS_PARENT(name);
    else
        n = resolve_symlink(name);
    open(n, flags, mode);
}

SUBST(void *, dlopen, (char const *path, int mode))
    my_dlopen(path, mode);
ENDSUBST

SUBST(int, openat, (int fd, char const *name, int flags, int mode))
    openat(fd, rs_at_flagged(fd, name, flags), flags, mode);
ENDSUBST

SUBST(int, lstat, (char const *path, struct stat *buf))
    lstat(RS_PARENT(path), buf);
ENDSUBST

SUBST(int, stat, (char const *path, struct stat *buf))
    stat(resolve_symlink(path), buf);
ENDSUBST

SUBST(int, fstatat, (int fd, char const *path, struct stat *buf, int flag))
    fstatat(fd, rs_at_flagged(fd, path, flag), buf, flag);
ENDSUBST

SUBST(int, access, (char const *path, int mode))
    access(resolve_symlink(path), mode);
ENDSUBST

SUBST(int, faccessat, (int fd, char const *path, int mode, int flag))
    faccessat(fd, rs_at_flagged(fd, path, flag), mode, flag);
ENDSUBST

SUBST(int, chflags, (char const *path, int flags))
    chflags(RS_PARENT(path), flags);
ENDSUBST

SUBST(int, mkfifo, (char const *path, mode_t mode))
    mkfifo(RS_PARENT(path), mode);
ENDSUBST

SUBST(int, chmod, (char const *path, mode_t mode))
    chmod(resolve_symlink(path), mode);
ENDSUBST

SUBST(int, fchmodat, (int fd, char const *path, mode_t mode, int flag))
    fchmodat(fd, rs_at_flagged(fd, path, flag), mode, flag);
ENDSUBST

SUBST(int, chown, (char const *path, uid_t owner, gid_t group))
    chown(resolve_symlink(path), owner, group);
ENDSUBST

SUBST(int, lchown, (char const *path, uid_t owner, gid_t group))
    lchown(RS_PARENT(path), owner, group);
ENDSUBST

SUBST(int, fchownat, (int fd, char const *path, uid_t owner, gid_t group, int flag))
    fchownat(fd, rs_at_flagged(fd, path, flag), owner, group, flag);
ENDSUBST

SUBST(int, link, (char const *path1, char const *path2))
    char newp1[PATH_MAX];
    strlcpy(newp1, RS_PARENT(path1), PATH_MAX);
    link(newp1, RS_PARENT(path2));
ENDSUBST

SUBST(int, linkat, 
        (int fd1, char const *path1, int fd2, char const *path2, int flag))
    const char *newp1 = (flag & AT_SYMLINK_FOLLOW) ? resolve_symlink_at(fd1, path1)
                                                   : resolve_symlink_parent(path1, fd1);
    linkat(fd1, newp1, fd2, resolve_symlink_parent(path2, fd2), flag);
ENDSUBST

SUBST(int, unlink, (char const *path))
    unlink(RS_PARENT(path));
ENDSUBST

SUBST(int, unlinkat, (int fd, char const *path, int flag))
    unlinkat(fd, resolve_symlink_parent(path, fd), flag);
ENDSUBST

SUBST(int, symlink, (char const *what, char const *path))
    symlink(what, RS_PARENT(path));
ENDSUBST

SUBST(int, symlinkat, (char const *what, int fd, char const *path))
    symlinkat(what, fd, rs_at_flagged(fd, path, 0));
ENDSUBST

SUBST(ssize_t, readlink, (char const *path, char *buf, size_t bsz))
    readlink(RS_PARENT(path), buf, bsz);
ENDSUBST

SUBST(ssize_t, readlinkat, (int fd, char const *path, char *buf, size_t bsz))
    readlinkat(fd, rs_at_flagged(fd, path, 0), buf, bsz);
ENDSUBST

SUBST(FILE *, fopen, (char const *path, char const *mode))
    fopen(resolve_symlink(path), mode);
ENDSUBST

SUBST(FILE *, freopen, (char const *path, char const *mode, FILE *orig))
    freopen(resolve_symlink(path), mode, orig);
ENDSUBST

SUBST(int, open, (char const *name, int flags, int mode))
    my_open(name, flags, mode);
ENDSUBST

SUBST(int, clonefile, (char const *path1, char const *path2, int flags))
    clonefile( (flags & CLONE_NOFOLLOW) ? RS_PARENT(path1)
                                        : resolve_symlink(path1)
             , RS_PARENT(path2), flags);
ENDSUBST

SUBST(int, clonefileat, 
        (int fd1, char const *path1, int fd2, char const *path2, int flags))
    clonefileat( fd1
               , (flags & CLONE_NOFOLLOW) ? resolve_symlink_parent(path1, fd1)
                                          : resolve_symlink_at(fd1, path1)
                , fd2
                , resolve_symlink_parent(path2, fd2)
                , flags);
        clonefileat(fd1, resolve_symlink_at(fd1, path1), fd2, resolve_symlink_parent(path2, fd2), flags);
ENDSUBST

SUBST(int, fclonefileat, (int src, int fd, char const *path, int flags))
    fclonefileat(src, fd, resolve_symlink_parent(path, fd), flags);
ENDSUBST

SUBST(int, exchangedata, (char const *path1, char const *path2, int options))
    exchangedata((options & FSOPT_NOFOLLOW) ? RS_PARENT(path1)
                                            : resolve_symlink(path1)
                , RS_PARENT(path2)
                , options);
ENDSUBST

SUBST(int, truncate, (char const *path, off_t length))
    truncate(resolve_symlink(path), length);
ENDSUBST

SUBST(int, utimes, (char const *path, struct timeval times[2]))
    utimes(resolve_symlink(path), times);
ENDSUBST

SUBST(int, rename, (char const *from, char const *to))
    char newp1[PATH_MAX];
    strlcpy(newp1, RS_PARENT(from), PATH_MAX);
    rename(newp1, RS_PARENT(to));
ENDSUBST

SUBST(int, my_renameat, (int fd1, char const *from, int fd2, char const *to))
    char newp1[PATH_MAX];
    strlcpy(newp1, resolve_symlink_parent(from, fd1), PATH_MAX);
    renameat(fd1, newp1, fd2, resolve_symlink_parent(to, fd2));
ENDSUBST

SUBST(int, renamex_np, (char const *from, char const *to, int flags))
    char newp1[PATH_MAX];
    strlcpy(newp1, RS_PARENT(from), PATH_MAX);
    renamex_np(newp1, RS_PARENT(to), flags);
ENDSUBST

SUBST(int, renameatx_np,
        (int fd1, char const *from, int fd2, char const *to, int flags))
    char newp1[PATH_MAX];
    strlcpy(newp1, resolve_symlink_parent(from, fd1), PATH_MAX);
    renameatx_np(fd1, newp1, fd2, resolve_symlink_parent(to, fd2), flags);
ENDSUBST

SUBST(int, undelete, (char const *path))
    undelete(resolve_symlink(path));
ENDSUBST

SUBST(int, mkdir, (char const *path, mode_t mode))
    mkdir(RS_PARENT(path), mode);
ENDSUBST

SUBST(int, mkdirat, (int fd, char const *path, mode_t mode))
    mkdirat(fd, resolve_symlink_parent(path, fd), mode);
ENDSUBST

SUBST(int, rmdir, (char const *path))
    rmdir(RS_PARENT(path));
ENDSUBST

SUBST(int, chdir, (char const *path))
    chdir(resolve_symlink(path));
ENDSUBST

SUBST(int, statfs, (char const *path, struct statfs *buf))
    statfs(resolve_symlink(path), buf);
ENDSUBST

SUBST(ssize_t, listxattr, 
        (char const *path, char *buf, size_t size, int options))
    listxattr(RS_PARENT(path), buf, size, options);
ENDSUBST

SUBST(int, removexattr, 
        (char const *path, char const *name, int options))
    removexattr(RS_PARENT(path), name, options);
ENDSUBST

SUBST(int, setxattr, 
        (char const *path, char const *name, void *value, size_t size, u_int32_t position, int options))
    setxattr(RS_PARENT(path), name, value, size, position, options);
ENDSUBST

SUBST(int, pathconf, (char const *path, int name))
    pathconf(RS_PARENT(path), name);
ENDSUBST

SUBST(int, setattrlist, 
        (char const *path, struct attrlist *attrList, void *attrBuf, size_t attrBufSize, unsigned long options))
    setattrlist(RS_PARENT(path), attrList, attrBuf, attrBufSize, options);
ENDSUBST

SUBST(int, getattrlist, 
        (char const *path, struct attrlist *attrList, void *attrBuf, size_t attrBufSize, unsigned long options))
    getattrlist(RS_PARENT(path), attrList, attrBuf, attrBufSize, options);
ENDSUBST

SUBST(int, getattrlistat, 
        (int fd, char const *path, struct attrlist *attrList, void *attrBuf, size_t attrBufSize, unsigned long options))
    getattrlistat(fd, resolve_symlink_parent(path, fd), attrList, attrBuf, attrBufSize, options);
ENDSUBST

char const *my_getcwd(char *buf, size_t size)
{
    getcwd(buf, size);
    strlcpy(buf, rewrite_path_rev(buf), size);
    DEBUG("getcwd() -> '%s' was called.", buf);
    return buf;
}

SUBST(DIR *, opendir, (char const *path))
    opendir(resolve_symlink(path));
ENDSUBST

// vim: ft=c.doxygen

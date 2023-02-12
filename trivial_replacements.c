#include "common.h"

int my_open(char *name, int flags, int mode)
{
    DEBUG("open(%s) was called.", name);
    return open(rewrite_path(name), flags, mode);
}

int my_openat(int fd, char *name, int flags, int mode)
{
    DEBUG("openat(%s) was called.", name);
    return openat(fd, rewrite_path(name), flags, mode);
}

int my_lstat(char *path, struct stat *buf)
{
    DEBUG("lstat(%s) was called.", path);
    return lstat(rewrite_path(path), buf);
}

int my_stat(char *path, struct stat *buf)
{
    DEBUG("stat(%s) was called.", path);
    return stat(rewrite_path(path), buf);
}

int my_fstatat(int fd, char *path, struct stat *buf, int flag)
{
    DEBUG("fstatat(%s) was called.", path);
    return fstatat(fd, rewrite_path(path), buf, flag);
}

int my_access(char *path, int mode)
{
    DEBUG("access(%s) was called.", path);
    return access(rewrite_path(path), mode);
}

int my_faccessat(int fd, char *path, int mode, int flag)
{
    DEBUG("faccessat(%s) was called.", path);
    return faccessat(fd, rewrite_path(path), mode, flag);
}

int my_chflags(char *path, int flags)
{
    DEBUG("chflags(%s) was called.", path);
    return chflags(rewrite_path(path), flags);
}

DIR *my_opendir(char *path)
{
    DEBUG("opendir(%s) was called.", path);
    return opendir(rewrite_path(path));
}

#include <dirent.h>
#include <sys/types.h>

/**
 * @file        trivial_replacements.h
 * @author      Karim Vergnes <me@thesola.io>
 * @copyright   GPLv2
 * @brief       Directory of syscall replacements definitions.
 *
 * This header file is used in @ref fakedir.c to populate the interpose table,
 * as well as to dogfood our modified syscalls in @ref my_execve.
 * It should reflect the contents of @ref trivial_replacements.c exactly.
 */

int my_open(char const *name, int flags, int mode);
int my_openat(int fd, char const *name, int flags, int mode);
int my_lstat(char const *path, struct stat *buf);
int my_stat(char const *path, struct stat *buf);
int my_fstatat(int fd, char const *path, struct stat *buf, int flag);
int my_access(char const *path, int mode);
int my_faccessat(int fd, char const *path, int mode, int flag);
int my_chflags(char const *path, int flags);
int my_mkfifo(char const *path, mode_t mode);
int my_chmod(char const *path, mode_t mode);
int my_fchmodat(int fd, char const *path, mode_t mode, int flag);
int my_chown(char const *path, uid_t owner, gid_t group);
int my_lchown(char const *path, uid_t owner, gid_t group);
int my_fchownat(int fd, char const *path, uid_t owner, gid_t group, int flag);
int my_link(char const *path1, char const *path2);
int my_linkat(int fd1, char const *path1, int fd2, char const *path2, int flag);
int my_unlink(char const *path);
int my_unlinkat(int fd, char const *path, int flag);
int my_symlink(char const *what, char const *path);
int my_symlinkat(char const *what, int fd, char const *path);
ssize_t my_readlink(char const *path, char *buf, size_t bsz);
ssize_t my_readlinkat(int fd, char const *path, char *buf, size_t bsz);
int my_clonefile(char const *path1, char const *path2, int flags);
int my_clonefileat(int fd1, char const *path1, int fd2, char const *path2, int flags);
int my_fclonefileat(int src, int fd, char const *path, int flags);
int my_exchangedata(char const *path1, char const *path2, int options);
int my_truncate(char const *path, off_t length);
int my_utimes(char const *path, struct timeval times[2]);
int my_rename(char const *from, char const *to);
int my_renameat(int fd1, char const *from, int fd2, char const *to);
int my_renamex_np(char const *from, char const *to, int flags);
int my_renameatx_np(int fd1, char const *from, int fd2, char const *to, int flags);
int my_undelete(char const *path);
int my_mkdir(char const *path, mode_t mode);
int my_mkdirat(int fd, char const *path, mode_t mode);
int my_rmdir(char const *path);
int my_chdir(char const *path);
char const *my_getcwd(char *buf, size_t size);

DIR *my_opendir(char const *path);

// vim: ft=c.doxygen

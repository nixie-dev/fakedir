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

int my_open(char *name, int flags, int mode);
int my_openat(int fd, char *name, int flags, int mode);
int my_lstat(char *path, struct stat *buf);
int my_stat(char *path, struct stat *buf);
int my_fstatat(int fd, char *path, struct stat *buf, int flag);
int my_access(char *path, int mode);
int my_faccessat(int fd, char *path, int mode, int flag);
int my_chflags(char *path, int flags);
int my_mkfifo(char *path, mode_t mode);
int my_chmod(char *path, mode_t mode);
int my_fchmodat(int fd, char *path, mode_t mode, int flag);
int my_chown(char *path, uid_t owner, gid_t group);
int my_lchown(char *path, uid_t owner, gid_t group);
int my_fchownat(int fd, char *path, uid_t owner, gid_t group, int flag);
int my_link(char *path1, char *path2);
int my_linkat(int fd1, char *path1, int fd2, char *path2, int flag);
int my_unlink(char *path);
int my_unlinkat(int fd, char *path, int flag);
int my_symlink(char *what, char *path);
int my_symlinkat(char *what, int fd, char *path);
ssize_t my_readlink(char *path, char *buf, size_t bsz);
ssize_t my_readlinkat(int fd, char *path, char *buf, size_t bsz);
int my_clonefile(char *path1, char *path2, int flags);
int my_clonefileat(int fd1, char *path1, int fd2, char *path2, int flags);
int my_fclonefileat(int src, int fd, char *path, int flags);
int my_exchangedata(char *path1, char *path2, int options);
int my_truncate(char *path, off_t length);
int my_utimes(char *path, struct timeval times[2]);
int my_rename(char *from, char *to);
int my_renameat(int fd1, char *from, int fd2, char *to);
int my_renamex_np(char *from, char *to, int flags);
int my_renameatx_np(int fd1, char *from, int fd2, char *to, int flags);
int my_undelete(char *path);
int my_mkdir(char *path, mode_t mode);
int my_mkdirat(int fd, char *path, mode_t mode);
int my_rmdir(char *path);
int my_chdir(char *path);
char *my_getcwd(char *buf, size_t size);

DIR *my_opendir(char *path);

// vim: ft=c.doxygen

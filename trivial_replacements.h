#include <dirent.h>

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
DIR *my_opendir(char *path);

// vim: ft=c.doxygen

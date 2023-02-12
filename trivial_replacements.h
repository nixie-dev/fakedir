#include <dirent.h>

int my_open(char *name, int flags, int mode);
int my_openat(int fd, char *name, int flags, int mode);
int my_lstat(char *path, struct stat *buf);
int my_stat(char *path, struct stat *buf);
int my_fstatat(int fd, char *path, struct stat *buf, int flag);
int my_access(char *path, int mode);
int my_faccessat(int fd, char *path, int mode, int flag);
int my_chflags(char *path, int flags);
DIR *my_opendir(char *path);

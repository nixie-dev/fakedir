#include <stdio.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/syslimits.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>

extern bool isdebug;

#define DEBUG(p, args...) \
    if (isdebug) dprintf(2, p "\n", ## args)

char *rewrite_path(char *path);

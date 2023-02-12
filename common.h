#include <stdio.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/syslimits.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>

/**
 * @file        common.h
 * @author      Karim Vergnes <me@thesola.io>
 * @copyright   GPLv2
 * @brief       Common headers to the fakedir library.
 *
 * This header file imports original definitions for all replaced system calls
 * and defines the primary functions.
 */

/**
 * @brief   Flag to enable debug printing. Set by FAKEDIR_DEBUG env var.
 * @see DEBUG   Debug printing macro which uses this variable
 */
extern bool isdebug;

/**
 * @brief   Prints to stderr if FAKEDIR_DEBUG is set.
 * @param p     Format string for @ref printf
 * @param args  Additional arguments for @ref printf
 */
#define DEBUG(p, args...) \
    if (isdebug) dprintf(2, p "\n", ## args)

/**
 * This function compares the start of a path string with FAKEDIR_PATTERN.
 * If there is a match, the rest of the path is appended to the path buffer,
 * functionally replacing FAKEDIR_PATTERN with FAKEDIR_TARGET in the path.
 *
 * @brief   Rewrites start of path according to env vars.
 * @param path  The path to be rewritten
 * @return  A string representing the rewritten path.
 */
char *rewrite_path(char *path);

// vim: ft=c.doxygen

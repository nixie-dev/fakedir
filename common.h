#include <stdio.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/syslimits.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>

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

/**
 * This function resolves all symbolic links through the given path down
 * to the last parent directory. This is useful for open() with O_SYMLINK,
 * so that the last symbolic link is preserved.
 *
 * @brief   Resolve symbolic links in path down to parent directory.
 * @param path  The path to perform resolution on
 * @param fd    The file descriptor to pass back to @ref resolve_symlink_at,
 *              or -1 to use @ref resolve_symlink.
 * @return  A string representing the resolved path.
 * @see Mutually recursive with @ref resolve_symlink and @ref resolve_symlink_at
 */
char *resolve_symlink_parent(char *path, int fd);

/**
 * This function performs a recursive resolution of all symbolic links
 * through the given path, including the final file if it is a symbolic link.
 * This is used by open() without O_SYMLINK, O_NOFOLLOW or O_NOFOLLOW_ANY,
 * as well as l-variants of certain calls (like lchown, lstat)
 *
 * @brief   Resolve all symbolic links in path, taking fakedir into account.
 * @param path  The path to perform resolution on
 * @return  A string representing the resolved path
 * @see Mutually recursive with @ref resolve_symlink_parent
 */
char *resolve_symlink(char *path);

/**
 * This function is an alternative entry point into the recursive symbolic
 * link resolution system, for use with -at-variants of certain syscalls.
 *
 * @brief   Like resolve_symlink, but with file descriptor as current dir.
 * @param fd    The file descriptor representing the current directory.
 * @param path  The path to perform resolution on.
 * @return  A string representing the resolved path
 * @see Functionally equivalent to @ref resolve_symlink
 * @see Mutually recursive with @ref resolve_symlink_parent
 */
char *resolve_symlink_at(int fd, char *path);

// vim: ft=c.doxygen

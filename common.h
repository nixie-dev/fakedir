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
#include <sys/clonefile.h>
#include <sys/attr.h>
#include <sys/time.h>
#include <sys/param.h>
#include <sys/mount.h>
#include <sys/xattr.h>
#include <stdint.h>
#include <spawn.h>
#include <dlfcn.h>
#include <pthread.h>

#include <mach-o/dyld.h>

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
 * @brief   Strictly locks syscall rewrite operations.
 */
extern pthread_mutex_t _lock;

/**
 * @brief   Dynamically loaded path to ourselves, for preservation across exec
 * @see execve_patch_envp uses this variable
 */
extern const char *ownpath;

/**
 * @brief   Value of FAKEDIR_PATTERN, the fake directory.
 */
extern const char *pattern;

/**
 * @brief   Value of FAKEDIR_TARGET, the real directory to rewrite pattern to.
 */
extern const char *target;

/**
 * @brief   Special constant to tell pspawn_patch_envp() to use execve()
 *          instead.
 */
#define PSP_EXEC -15

#ifndef STRIP_DEBUG
/**
 * @brief   Prints to stderr if FAKEDIR_DEBUG is set.
 * @param p     Format string for @ref printf
 * @param args  Additional arguments for @ref printf
 */
# define DEBUG(p, args...) \
    if (isdebug) dprintf(2, "[fakedir] " p "\n", ## args)

#else
# define DEBUG(p, args...) ;
#endif

/**
 * This function parses a Mach-O binary, and calls a function for each library
 * found that needs rewriting.
 *
 * @param path  The path to the binary to parse
 * @param step  The handler to call on match
 */
void macho_add_dependencies(char const *path, void (*step)(char const *d));

/**
 * This function compares the start of a path string with FAKEDIR_PATTERN.
 * If there is a match, the rest of the path is appended to the path buffer,
 * functionally replacing FAKEDIR_PATTERN with FAKEDIR_TARGET in the path.
 *
 * @brief   Rewrites start of path according to env vars.
 * @param path  The path to be rewritten
 * @return  A string representing the rewritten path.
 */
char const *rewrite_path(char const *path);

/**
 * This function performs the reverse operation to @ref rewrite_path, that is
 * it replaces FAKEDIR_TARGET with FAKEDIR_PATTERN at the start of the given
 * path.
 *
 * @brief   Performs inverse of rewrite_path
 * @param path  The path to be rewritten
 * @return  A string represneting the rewritten path.
 */
char const *rewrite_path_rev(char const *path);

/**
 * This function resolves all symbolic links through the given path down
 * to the last parent directory. This is useful for open() with O_SYMLINK,
 * so that the last symbolic link is preserved.
 *
 * @brief   Resolve symbolic links in path down to parent directory.
 * @param fd    The file descriptor to pass back to @ref resolve_symlink_at,
 *              or -1 to use @ref resolve_symlink.
 * @param path  The path to perform resolution on
 * @return  A string representing the resolved path.
 * @see Mutually recursive with @ref resolve_symlink and @ref resolve_symlink_at
 */
char const *resolve_symlink_parent(int fd, char const *path);

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
#define resolve_symlink(path) resolve_symlink_at(-1, path)

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
char const *resolve_symlink_at(int fd, char const *path);

/**
 * This function performs a limited length match against msg. If the contents
 * of msg match those of pat until the end of pat, the function returns true.
 *
 * @brief   Check if a string starts with another string.
 * @param pat   The pattern to use for comparison count.
 * @param msg   The string to check against.
 * @return  True if the start of msg is the contents of pat, False otherwise.
 */
bool startswith(char const *pat, char const *msg);

/**
 * This function performs a limited length match against the end of msg. If the
 * contents of the end of msg match those of pat until the end of both, the
 * function returns true.
 *
 * @brief   Check if a string ends with another string.
 * @param pat   The pattern to use for comparison count and offset.
 * @param msg   The string to check against.
 * @return  True if the end of msg is the contents of pat, False otherwise.
 */
bool endswith(char const *pattern, char const *msg);

// vim: ft=c.doxygen

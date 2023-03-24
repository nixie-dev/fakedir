#include "common.h"
#include "trivial_replacements.h"
#include "execve.h"


/**
 * @file        fakedir.c
 * @author      Karim Vergnes <me@thesola.io>
 * @copyright   GPLv2
 * @brief       Injection library to fake a directory existing elsewhere.
 *
 * This library swaps system calls which take a path as argument for versions
 * where the specified substitution exists.
 * The end result is a virtual filesystem tree where TARGET exists at PATTERN
 * but will not show up in a directory listing.
 *
 * Due to the sensitive nature of these system calls, heap memory allocation is
 * strictly forbidden. All code below must use memory allocated on the stack.
 */


bool isdebug = false;

const char *ownpath;

static char pathbuf[PATH_MAX];
static char rpathbuf[PATH_MAX];
static char linkbuf[PATH_MAX];
static char dedupbuf[PATH_MAX];

const char *pattern;
const char *target;

__attribute__((constructor))
static void __fakedir_init(void)
{
    isdebug = getenv("FAKEDIR_DEBUG");
    pattern = getenv("FAKEDIR_PATTERN");
    target = getenv("FAKEDIR_TARGET");
    if (! (pattern && target)) {
        dprintf(2, "Variables FAKEDIR_PATTERN and FAKEDIR_TARGET must be set.\n");
        exit(1);
    } else if (! (pattern[0] == '/' && target[0] == '/')) {
        dprintf(2, "Variables FAKEDIR_PATTERN and FAKEDIR_TARGET must be absolute paths.\n");
        exit(1);
    } else if (startswith(pattern, target)) {
        dprintf(2, "Variable FAKEDIR_PATTERN may not be a subset of FAKEDIR_TARGET.\n");
        exit(1);
    }

#   define selfname "libfakedir.dylib"
    int nimgs = _dyld_image_count();
    for (int i = 1; i < nimgs; i++) {
        ownpath = _dyld_get_image_name(i);
        if (! strncmp( ownpath + strlen(ownpath) - strlen(selfname)
                     , selfname
                     , strlen(selfname)))
            break;
    }
    DEBUG("I think I am '%s'", ownpath);

    strlcpy(pathbuf, target, PATH_MAX);
    strlcpy(rpathbuf, pattern, PATH_MAX);
    DEBUG("Initialized libfakedir with subtitution '%s' => '%s'", pattern, target);
}

bool startswith(char const *pattern, char const *msg)
{
    return ! strncmp(msg, pattern, strlen(pattern));
}

char const *rewrite_path(char const *path)
{
    if (pattern && startswith(pattern, path)) {
        strlcpy(pathbuf + strlen(target), path + strlen(pattern), PATH_MAX);
        DEBUG("Matched path '%s' to '%s'", path, pathbuf);
        return pathbuf;
    } else {
        if (path != dedupbuf)
            strlcpy(dedupbuf, path, PATH_MAX);
        return dedupbuf;
    }
}

char const *rewrite_path_rev(char const *path)
{
    if (target && startswith(target, path)) {
        strlcpy(rpathbuf + strlen(pattern), path + strlen(target), PATH_MAX);
        DEBUG("Reverse-matched path '%s' to '%s'", path, rpathbuf);
        return rpathbuf;
    } else {
        if (path != dedupbuf)
            strlcpy(dedupbuf, path, PATH_MAX);
        return dedupbuf;
    }
}

char const *resolve_symlink_parent(char const *path, int fd)
{
    char workpath[PATH_MAX];
    char *fname = NULL;

    strlcpy(workpath, path, PATH_MAX);
    for (int i = strlen(workpath); i > 0; i--) {
        if (workpath[i] == '/') {
            workpath[i] = 0;
            fname = workpath + i + 1;
            break;
        }
    }
    if (!fname) {
        strlcpy(linkbuf, rewrite_path(path), PATH_MAX);
        return linkbuf;
    }

    if (fd >= 0)
        resolve_symlink_at(fd, workpath);
    else
        resolve_symlink(workpath);
    strlcat(linkbuf, "/", PATH_MAX);
    strlcat(linkbuf, fname, PATH_MAX);
    return rewrite_path(linkbuf);
}

char const *resolve_symlink(char const *path)
{
    char wpath[PATH_MAX];
    strlcpy(wpath, path, PATH_MAX);
    // There is a chance we're being fed our own work buffer, save it.

    ssize_t linklen = readlink(rewrite_path(path), linkbuf, PATH_MAX);
    //                ^^^^^^^^^^^^^^^^^^^^^^^^^^^ look familiar?
    //                  well, nope. This, unlike my_readlink, is an atom.
    //                  If you were to use my_readlink here instead, don't.
    //                  The resulting recursion model would fry my brain.

    if (linklen < 0) {
        // Symlink resolution failed, recurse through path
        char const *result = resolve_symlink_parent(path, -1);
        DEBUG("Symbolic link '%s' recursively resolved to '%s'", path, result);
        return result;
    }
    linkbuf[linklen] = 0;
    if (linkbuf[0] != '/') {
        // Symlink is relative, copy it to end of buffer then rewrite parent
        int off = strlen(wpath);
        for (; off > 0; off--)
            if (wpath[off] == '/')
                break;
        for (int i = off + linklen; i >= 0; i--)
            linkbuf[i + off + 1] = linkbuf[i];
        for (int i = off; i >= 0; i--)
            linkbuf[i] = wpath[i];
    }
    char const *result = rewrite_path(linkbuf);
    DEBUG("Symbolic link '%s' resolved to '%s'", wpath, result);
    return resolve_symlink(result);
}

char const *resolve_symlink_at(int fd, char const *path)
{
    char wpath[PATH_MAX];
    strlcpy(wpath, path, PATH_MAX);

    ssize_t linklen = readlinkat(fd, rewrite_path(path), linkbuf, PATH_MAX);

    if (linklen < 0) {
        // Symlink resolution failed, recurse through path
        char const *result = resolve_symlink_parent(path, fd);
        DEBUG("Symbolic link '%s' recursively fd-resolved to '%s'", path, result);
        return result;
    }
    linkbuf[linklen] = 0;
    if (linkbuf[0] != '/') {
        // Symlink is relative, copy it to end of buffer then rewrite parent
        int off = strlen(wpath);
        for (; off > 0; off--)
            if (wpath[off] == '/')
                break;
        for (int i = off + linklen; i >= 0; i--)
            linkbuf[i + off + 1] = linkbuf[i];
        for (int i = off; i >= 0; i--)
            linkbuf[i] = wpath[i];
    }
    char const *result = rewrite_path(linkbuf);
    DEBUG("Symbolic link '%s' fd-resolved to '%s'", wpath, result);
    return resolve_symlink_at(fd, result);
}

int my_execve(char const *path, char *argv[], char *envp[])
{
    DEBUG("execve(%s) was called.", path);
    int tgt = my_open(path, O_RDONLY, 0000);
    char shebang[PATH_MAX];

    // Since we're overriding shebang behavior, we might allow non-executables
    // to be run with a shebang. To prevent that, we skip the shebang parser
    // in files without exec rights and let the real execve() return the error.
    int canexec = ! my_access(path, X_OK);
    read(tgt, shebang, PATH_MAX);
    close(tgt);

    if (canexec && !strncmp(shebang, "#!", 2)) {
        DEBUG("Executable '%s' has a shebang, parsing...", path);
        return execve_parse_shebang(shebang, argv, envp);
    }

    return execve_patch_envp(resolve_symlink(path), argv, envp);
}

#pragma clang diagnostic ignored "-Wdeprecated-declarations"
__attribute__((used, section("__DATA,__interpose")))
static void *interpose[] =  { my_open       , open
                            , my_openat     , openat
                            , my_execve     , execve
                            , my_lstat      , lstat
                            , my_stat       , stat
#ifdef __x86_64__
                            , my_lstat      , lstat64
                            , my_stat       , stat64
#endif
                            , my_fstatat    , fstatat
                            , my_access     , access
                            , my_faccessat  , faccessat
                            , my_opendir    , opendir
                            , my_chflags    , chflags
                            , my_mkfifo     , mkfifo
                            , my_chmod      , chmod
                            , my_fchmodat   , fchmodat
                            , my_chown      , chown
                            , my_lchown     , lchown
                            , my_fchownat   , fchownat
                            , my_link       , link
                            , my_linkat     , linkat
                            , my_unlink     , unlink
                            , my_unlinkat   , unlinkat
                            , my_symlink    , symlink
                            , my_symlinkat  , symlinkat
                            , my_readlink   , readlink
                            , my_readlinkat , readlinkat
                            , my_clonefile  , clonefile
                            , my_clonefileat, clonefileat
                            , my_fclonefileat, fclonefileat
                            , my_exchangedata, exchangedata
                            , my_truncate   , truncate
                            , my_utimes     , utimes
                            , my_rename     , rename
                            , my_renameat   , renameat
                            , my_renamex_np , renamex_np
                            , my_renameatx_np, renameatx_np
                            , my_undelete   , undelete
                            , my_mkdir      , mkdir
                            , my_mkdirat    , mkdirat
                            , my_rmdir      , rmdir
                            , my_chdir      , chdir
                            , my_statfs     , statfs
                            , my_statfs     , statfs64
                            , my_getcwd     , getcwd
                            };

// vim: ft=c.doxygen

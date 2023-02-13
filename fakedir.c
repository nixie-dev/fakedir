#include "common.h"
#include "trivial_replacements.h"

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

static char pathbuf[PATH_MAX];
static char linkbuf[PATH_MAX];

static char *pattern;
static char *target;

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
    }

    strlcpy(pathbuf, target, PATH_MAX);
    DEBUG("Initialized libfakedir with subtitution '%s' => '%s'", pattern, target);
}

char *rewrite_path(char *path)
{
    if (pattern && !strncmp(path, pattern, strlen(pattern))) {
        strlcpy(pathbuf + strlen(target), path + strlen(pattern), PATH_MAX);
        DEBUG("Matched path '%s' to '%s'", path, pathbuf);
        return pathbuf;
    } else {
        return path;
    }
}

char *resolve_symlink_parent(char *path, int fd)
{
    char workpath[PATH_MAX];
    char *fname = NULL;

    strlcpy(workpath, path, PATH_MAX);
    for (int i = strlen(workpath); i > 0; i--) {
        if (workpath[i] == '/') {
            workpath[i] = 0;
            fname = workpath + i + 1;
            DEBUG("Preserved filename as '%s'", fname);
            break;
        }
    }
    DEBUG("Resolving symbolic link part '%s'", workpath);
    if (!fname) {
        strlcpy(linkbuf, path, PATH_MAX);
        return path;
    }

    if (fd >= 0)
        resolve_symlink_at(fd, workpath);
    else
        resolve_symlink(workpath);
    strlcat(linkbuf, "/", PATH_MAX);
    strlcat(linkbuf, fname, PATH_MAX);
    DEBUG("Rebuilt path is '%s'", linkbuf);
    return rewrite_path(linkbuf);
}

char *resolve_symlink(char *path)
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
        char *result = resolve_symlink_parent(path, -1);
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
    DEBUG("Symbolic link '%s' resolved to '%s'", wpath, linkbuf);
    return resolve_symlink(rewrite_path(linkbuf));
}

char *resolve_symlink_at(int fd, char *path)
{
    char wpath[PATH_MAX];
    strlcpy(wpath, path, PATH_MAX);

    ssize_t linklen = readlinkat(fd, rewrite_path(path), linkbuf, PATH_MAX);

    if (linklen < 0) {
        // Symlink resolution failed, recurse through path
        char *result = resolve_symlink_parent(path, fd);
        DEBUG("Symbolic link '%s' recursively fd-resolved to '%s'", path, result);
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
    DEBUG("Symbolic link '%s' fd-resolved to '%s'", wpath, linkbuf);
    return resolve_symlink_at(fd, rewrite_path(linkbuf));
}

static int execve_patch_envp(char *path, char *argv[], char *envp[])
{
    DEBUG("Preparing envp for fakedir library loading");
    int envc = 0;
    int dsr_idx = -1;   // DYLD_SHARED_REGION
    int dscd_idx = -1;  // DYLD_SHARED_CACHE_DIR
    int dil_idx = -1;   // DYLD_INSERT_LIBRARIES

#   define dsr_match "DYLD_SHARED_REGION="
#   define dscd_match "DYLD_SHARED_CACHE_DIR="
#   define dil_match "DYLD_INSERT_LIBRARIES="

    char *dsr_full = dsr_match "private";
    char dscd_full[strlen(dscd_match) + PATH_MAX];
    char dil_full[strlen(dil_match) + PATH_MAX];
    strncpy(dscd_full, dscd_match, strlen(dscd_match));
    strncpy(dil_full, dil_match, strlen(dil_match));

    strlcat(dscd_full, getenv("HOME"), PATH_MAX);
    strlcat(dscd_full, "/Library/fakedir-dyld", PATH_MAX);

    for (; envp[envc]; envc++)
        ;
    char *new_envp[envc + 4];
    for (int i = 0; i < envc; i++) {
        if (! strncmp(envp[i], dil_match, strlen(dil_match)))
            dil_idx = i;
        else if (! strncmp(envp[i], dsr_match, strlen(dsr_match)))
            dsr_idx = i;
        else if (! strncmp(envp[i], dscd_match, strlen(dscd_match)))
            dscd_idx = i;
        new_envp[i] = envp[i];
    }

    //TODO: build actual shared cache, probably by capturing all fakedir
    // children and rewriting *.map accordingly
    //TODO: Locate and carry ourselves back into DYLD_INSERT_LIBRARIES
    //TODO: Test injection without DYLD_SHARED_REGION=private to see if
    // copying the system cache is needed

    new_envp[dscd_idx == -1 ? envc++ : dscd_idx] = dscd_full;
    new_envp[dil_idx == -1 ? envc++ : dil_idx] = dil_full;
    new_envp[dsr_idx == -1 ? envc++ : dsr_idx] = dsr_full;

    return execve(path, argv, new_envp);
}

int my_execve(char *path, char *argv[], char *envp[])
{
    //TODO: trick the linker into loading libs in our fakedir
    // We might actually be able to do that by building a shared cache where
    // libraries are bound without "existing", just like macOS itself does.
    // So, for instance, if my program wants /nix/store/xxx-lol/lib/lol.dylib
    // then we inject the real library under the fake path in our binary cache
    // before executing.
    // Thus we need to add DYLD_SHARED_REGION=private, copy system shared cache
    // and use DYLD_SHARED_CACHE_DIR=$HOME/Library/fakedir-dyld or something.

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
        char my_path[PATH_MAX]; // This will be the path to our interpreter
        char my_args[PATH_MAX]; // This will be the following argument (if any)
                                // UNIX shebangs only permit a single argument
                                // before the actual file, which makes our job
                                // way easier.

        int path_start = 2;
        int path_len = 0;       // Start and length of interpreter path
        int args_start = 0;
        int args_len = 0;       // Start and length of interpreter argument
        int our_argc = 0;       // Number of arguments in original argv

        // Strip leading spaces between shebang and interpreter
        for (; path_start < PATH_MAX; path_start++)
            if (shebang[path_start] != ' ')
                break;
        // Determine interpreter length at first space or newline
        for (; path_len < PATH_MAX; path_len++)
            if (shebang[path_start + path_len] == ' '
              || shebang[path_start + path_len] == '\n')
                break;

        // Determine start of argument at first non-space after interpreter
        for (args_start = path_start + path_len; args_start < PATH_MAX; args_start++)
            if (shebang[args_start] != ' ')
                break;

        // Set argument length to the end of the shebang line...
        for (; args_len < PATH_MAX; args_len++)
            if (shebang[args_start + args_len] == '\n')
                break;

        // then backtrack to strip trailing spaces from it.
        for (; args_len > 0; args_len--)
            if (shebang[args_start + args_len - 1] != ' ')
                break;

        // Finally, count arguments in original argv...
        for (; our_argc < ARG_MAX; our_argc++)
            if (argv[our_argc] == NULL)
                break;

        // ... so that we can stack-allocate our new one.
        char *new_argv[our_argc + 2];

        // Populate path and arg strings then formulate execve()
        strncpy(my_path, shebang + path_start, path_len);
        my_path[path_len] = 0;
        strncpy(my_args, shebang + args_start, args_len);
        my_args[args_len] = 0;

        DEBUG("Parsed interpreter '%s' with argument '%s'", my_path, my_args);
        new_argv[0] = my_path;
        if (args_len) {
            // With an argument
            new_argv[1] = my_args;
            for (int i = 0; argv[i]; i++)
                new_argv[i + 2] = argv[i];
            new_argv[our_argc + 2] = 0;
        } else {
            // Without argument
            for (int i = 0; argv[i]; i++)
                new_argv[i + 1] = argv[i];
            new_argv[our_argc + 1] = 0;
        }

        // Fun fact: shebangs are recursive! Scripts can be interpreters too!
        return my_execve(resolve_symlink(my_path), new_argv, envp);
    }

    return execve_patch_envp(resolve_symlink(path), argv, envp);
}

#pragma clang diagnostic ignored "-Wdeprecated-declarations"
__attribute__((used, section("__DATA,__interpose")))
static void *interpose[] =  { my_open       , open
                            , my_openat     , openat
                            , my_execve     , execve
                            , my_lstat      , lstat
                            , my_lstat      , lstat64
                            , my_stat       , stat
                            , my_stat       , stat64
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
                            };

// vim: ft=c.doxygen

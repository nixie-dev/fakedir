#include "common.h"
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

// Pulled from trivial_replacements. It's useful.
int my_open(char const *name, int flags, int mode);

bool _loaded = false;

bool isdebug = false;

const char *ownpath;

#ifndef STRIP_DEBUG
int debugfd = 2;
#endif

static char pathbuf[PATH_MAX];
static char rpathbuf[PATH_MAX];
static char linkbuf[PATH_MAX];
static char dedupbuf[PATH_MAX];
pthread_mutex_t _lock;

const char *pattern;
const char *target;

__attribute__((constructor))
static void __fakedir_init(void)
{
    if (_loaded)
        return;

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

#   if !defined(STRIP_DEBUG) && defined(DEBUG_FILE)
    char tgt[PATH_MAX];
    sprintf(tgt, "%s.%d", DEBUG_FILE, getpid());
    debugfd = open(tgt, O_CREAT|O_RDWR, 0644);
#   endif

#   ifdef STRIP_DEBUG
    if (isdebug)
        dprintf(2, "[fakedir] WARNING: This build was configured without debug messages.\n");
#   elif defined(ALWAYS_DEBUG)
    dprintf(2, "[fakedir] WARNING: This build was configured with mandatory debug messages.\n");
    isdebug = true;
#   endif

#   define selfname "libfakedir.dylib"
    int nimgs = _dyld_image_count();
    for (int i = 0; i < nimgs; i++) {
        ownpath = _dyld_get_image_name(i);
        if (endswith(selfname, ownpath))
            break;
    }
    DEBUG("I think I am '%s'", ownpath);

    strlcpy(pathbuf, target, PATH_MAX);
    strlcpy(rpathbuf, pattern, PATH_MAX);

    pthread_mutex_init(&_lock, NULL);
    DEBUG("Initialized libfakedir with subtitution '%s' => '%s'", pattern, target);
    _loaded = true;
}

__attribute__((destructor))
static void __fakedir_fini(void)
{
    DEBUG("Closing shop and deleting mutex.");
#   if !defined STRIP_DEBUG && defined(DEBUG_FILE)
    close(debugfd);
#   endif
    pthread_mutex_destroy(&_lock);
}

bool startswith(char const *pattern, char const *msg)
{
    if (strlen(pattern) > strlen(msg))
        return false;
    return ! strncmp(msg, pattern, strlen(pattern));
}

bool endswith(char const *pattern, char const *msg)
{
    if (strlen(pattern) > strlen(msg))
        return false;
    return ! strncmp(msg + strlen(msg) - strlen(pattern), pattern, strlen(pattern));
}

char const *rewrite_path(char const *path)
{
    if (startswith("/.", path))
        path += 2;
    if (pattern && startswith(pattern, path)) {
        strlcpy(pathbuf + strlen(target), path + strlen(pattern), PATH_MAX);
        return pathbuf;
    } else {
        if (path != dedupbuf)
            strlcpy(dedupbuf, path, PATH_MAX);
        return dedupbuf;
    }
}

char const *rewrite_path_rev(char const *path)
{
    if (startswith("/.", path))
        path += 2;
    if (target && startswith(target, path)) {
        strlcpy(rpathbuf + strlen(pattern), path + strlen(target), PATH_MAX);
        return rpathbuf;
    } else {
        if (path != dedupbuf)
            strlcpy(dedupbuf, path, PATH_MAX);
        return dedupbuf;
    }
}

char const *resolve_symlink_parent(int fd, char const *path)
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

    resolve_symlink_at(fd, workpath);
    strlcat(linkbuf, "/", PATH_MAX);
    strlcat(linkbuf, fname, PATH_MAX);
    return rewrite_path(linkbuf);
}

char const *resolve_symlink_at(int fd, char const *path)
{
    char wpath[PATH_MAX];
    strlcpy(wpath, path, PATH_MAX);

    ssize_t linklen;
    if (fd != -1)
        linklen = readlinkat(fd, rewrite_path(path), linkbuf, PATH_MAX);
    else
        linklen = readlink(rewrite_path(path), linkbuf, PATH_MAX);

    if (linklen < 0) {
        // Symlink resolution failed, recurse through path
        char const *result = resolve_symlink_parent(fd, path);
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
    return resolve_symlink_at(fd, result);
}

int my_posix_spawn(pid_t *pid, char const *path, const posix_spawn_file_actions_t *facts, const posix_spawnattr_t *attrp, char *argv[], char *envp[])
{
    if (pid != PSP_EXEC)
        DEBUG("posix_spawn(%s) was called.", path);
    int tgt = my_open(path, O_RDONLY, 0000);
    char shebang[PATH_MAX];

    // Since we're overriding shebang behavior, we might allow non-executables
    // to be run with a shebang. To prevent that, we skip the shebang parser
    // in files without exec rights and let the real execve() return the error.
    int canexec = ! access(resolve_symlink(path), X_OK);
    read(tgt, shebang, PATH_MAX);
    close(tgt);

    if (canexec && !strncmp(shebang, "#!", 2)) {
        DEBUG("Executable '%s' has a shebang, parsing...", path);
        return pspawn_parse_shebang(pid, resolve_symlink(path), shebang, facts, attrp, argv, envp);
    }

    return pspawn_patch_envp(pid, resolve_symlink(path), facts, attrp, argv, envp);
}


// vim: ft=c.doxygen

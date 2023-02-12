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

    strcpy(pathbuf, target);
    DEBUG("Initialized libfakedir with subtitution '%s' => '%s'", pattern, target);
}

char *rewrite_path(char *path)
{
    if (pattern && !strncmp(path, pattern, strlen(pattern))) {
        strcpy(pathbuf + strlen(target), path + strlen(pattern));
        DEBUG("Matched path '%s' to '%s'", path, pathbuf);
        return pathbuf;
    } else {
        return path;
    }
}

int my_execve(char *path, char *argv[], char *envp[])
{
    //TODO: trick the linker into loading libs in our fakedir

    DEBUG("execve(%s) was called.", path);
    int tgt = my_open(path, O_RDONLY, 0000);
    char shebang[PATH_MAX];

    // Since we're overriding shebang behavior, we might allow non-executables
    // to be run with a shebang. To prevent that, we skip the shebang parser
    // in files without exec rights and let the real execve() return the error.
    int canexec = ! my_access(rewrite_path(path), X_OK);
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
        return my_execve(rewrite_path(my_path), new_argv, envp);
    }

    return execve(rewrite_path(path), argv, envp);
}

#pragma clang diagnostic ignored "-Wdeprecated-declarations"
__attribute__((used, section("__DATA,__interpose")))
static void *interpose[] =  { my_open   , open
                            , my_openat , openat
                            , my_execve , execve
                            , my_lstat  , lstat
                            , my_lstat  , lstat64
                            , my_stat   , stat
                            , my_stat   , stat64
                            , my_fstatat, fstatat
                            , my_access , access
                            , my_faccessat, faccessat
                            , my_opendir, opendir
                            , my_chflags, chflags
                            };

// vim: ft=c.doxygen

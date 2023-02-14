#include "common.h"

#include <mach-o/dyld.h>
#include <mach-o/loader.h>

extern int my_execve(char *path, char *av[], char *ep[]);

static void macho_add_dependencies(char *path, char *dil)
{

}

int execve_patch_envp(char *path, char *argv[], char *envp[])
{
    DEBUG("Preparing envp for fakedir library loading");
    int envc = 0;
    int dil_idx = -1;   // DYLD_INSERT_LIBRARIES

#   define dil_match "DYLD_INSERT_LIBRARIES="
#   define dil_size strlen(dil_match) + (PATH_MAX * 10)

    char wpath[PATH_MAX];
    strlcpy(wpath, path, PATH_MAX);

    char dil_full[dil_size];
    memset(dil_full, 0, dil_size);
    strncpy(dil_full, dil_match, strlen(dil_match));

    for (; envp[envc]; envc++)
        ;
    char *new_envp[envc + 4];
    for (int i = 0; i < envc; i++) {
        if (startswith(dil_match, envp[i]))
            dil_idx = i;
        new_envp[i] = envp[i];
    }

    strlcat(dil_full, ownpath, dil_size);

    //TODO: Recursively read library dependents in requested binary and compile
    // DYLD_INSERT_LIBRARIES to include them all.
    // To avoid accidentally preloading a conflicting library version, we
    // need to backup DYLD_INSERT_LIBRARIES before our autogen and pass it to
    // the libfakedir-bound child to sort out.
    // We'll also be breaking the Nix derivation builder's clean-env policy,
    // so we need to absolutely make sure our hack won't break results.

    DEBUG("Running with %s", dil_full);

    new_envp[dil_idx == -1 ? envc++ : dil_idx] = dil_full;
    new_envp[envc] = 0;

    return execve(wpath, argv, new_envp);
}

int execve_parse_shebang(char *shebang, char *argv[], char *envp[])
{
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

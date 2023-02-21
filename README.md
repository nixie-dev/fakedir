# fakedir, because who needs namespaces anyway?

Fakedir is a simple library, meant to trick as many `libSystem` calls into believing a given directory exists in another place. To achieve that, we resolve symlinks and rewrite paths until the resulting system calls points towards the original directory. _(what do you mean, "malware"?)_

This project's intended purpose is for use within [nixie](https://github.com/thesola10/nixie), but it is generic enough for anyone to use.

## How to use

Fakedir only supports macOS. If you're on Linux or any other sensible operating system, look into `bwrap` or `unshare` instead.

Fakedir also does not work for notarized binaries (macOS builtins or signed app bundles), unless System Integrity Protection is disabled (not recommended).

In order to start using fakedir, you need to inject the library, then tell it which directory it should fake and where.

```sh
DYLD_INSERT_LIBRARIES=/path/to/libfakedir.dylib FAKEDIR_PATTERN=/nix FAKEDIR_TARGET=$HOME/Library/nix ./myprogram
```

In the above example, the directory at `~/Library/nix` is faked as root directory `/nix`.

## Configuration

As you've seen above, the only way to configure fakedir is through environment variables.

- `FAKEDIR_PATTERN` is the path that should be matched and replaced whenever accessed. **This parameter is mandatory and must be an absolute path.**
- `FAKEDIR_TARGET` is the actual path that should be used whenever `FAKEDIR_PATTERN` is met. **This parameter is mandatory and must be an absolute path.**
- `FAKEDIR_DEBUG` enables debug output to standard error. This variable's value is not taken into account, and debug output will be enabled as long as it is set. (i.e. `FAKEDIR_DEBUG=0` will still enable debug output).

## Pitfalls

As mentioned above, fakedir does not work on executables under System Integrity Protection, since macOS prevents library injection for those.

The faked directory will not appear in its parent's directory listing, as we do not attempt to modify `readdir()`'s output.

Cyclic symbolic links are not detected and will result in a stack overflow, due to limitations with the in-library resolver.

The fakedir library itself may not be located in the fake directory.

Calling fakedir's `execve()` may result in a very large `DYLD_INSERT_LIBRARIES` environment variable, as we currently cannot hook into `dyld`'s own understanding of the filesystem.

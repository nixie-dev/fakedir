/**
 * @file        execve.h
 * @author      Karim Vergnes <me@thesola.io>
 * @copyright   GPLv2
 * @brief       Functions that recreate execve() logic with fake directory.
 *
 * This header file defines the functions used in our implementation of execve.
 * In order to achieve a realistic fake directory, we must be able to load
 * script interpreters from said directory, which requires us to re-implement
 * the shebang parser entirely.
 * In addition, fooled programs may designate libraries which only exist in our
 * fake directory, so we must be able to tell the dynamic linker where to find
 * those, through rewriting DYLD_INSERT_LIBRARIES.
 */

/**
 * This function is responsible for parsing a Mach-O binary, and launching it
 * with all required libraries located in our fakedir resolved and appended
 * to DYLD_INSERT_LIBRARIES. It also ensures that fakedir remains in that same
 * environment variable no matter what.
 *
 * @brief   Parses and appends libs in fakedir to environment, then executes.
 * @param path  The path to the executable to launch.
 * @param argv  The array of arguments to launch with, zero-terminated.
 * @param envp  The array of environment entries to use as reference, the
 *              actual environment set will be modified upon exec.
 * @return  Only on failure, the code that execve() returns.
 */
int execve_patch_envp(char *path, char *argv[], char *envp[]);

/**
 * This function is responsible for parsing the hash-bang line at the start of
 * a script file, in order to figure out the interpreter and its arguments,
 * then invokes the interpreter the same way the system would. The reason this
 * is required is to allow for a rewrite of the interpreter's own path, in the
 * event that said interpreter is located in our fakedir.
 *
 * @brief   Parses shebang and calls script interpreter with arguments.
 * @param bang  The string containing the shebang line (starting with #!).
 * @param argv  The array of arguments for the script, to use as reference.
 *              argv[0] must be the correct path to the script, as it will
 *              be carried over as-is to the interpreter.
 * @param envp  The array of environment entries to launch with zero-terminated
 * @return  Only on failure, the code that execve() returns.
 */
int execve_parse_shebang(char *bang, char *argv[], char *envp[]);

// vim: ft=c.doxygen

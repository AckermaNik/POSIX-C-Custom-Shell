# POSIX-C-Custom-Shell

A small Unix-style command shell written in C. It reads commands interactively, runs external programs by creating child processes with `fork()` and replacing them with `execv()`, and implements basic sequential commands, pipelines, and input/output redirection using POSIX process and file-descriptor calls.

## How command execution works

The shell does not use the C `system()` function. Instead, `shell.c` performs the process-management steps directly:

1. **Read a command:** `fgets()` reads a line from standard input. The prompt uses `getcwd()` for the current directory and `getenv("USER")` for the username. End-of-file (for example, Ctrl-D in a terminal) exits the input loop.
2. **Split the line:** `strtok()` separates commands, arguments, semicolon-delimited command sequences, and the supported pipe/redirection operators.
3. **Handle built-ins in the shell process:** `quit` exits the shell. `chdir` calls `chdir()` directly in the parent process so a directory change remains in effect for the next prompt. Running `chdir` in a child would change only that child process's directory.
4. **Launch external commands:** For an ordinary external command, `fork()` creates a child process. The child constructs `/bin/<command>` and calls `execv(path, arguments)`. A successful `execv()` replaces the child process image with the requested program. The parent calls `wait()` to wait for the child before showing the next prompt.

This is a direct example of the Unix process model: `fork()` creates a process, `execv()` loads a program into that process, and `wait()` synchronizes the parent with its child. Since the shell constructs paths under `/bin/` itself, this implementation does not search the `PATH` environment variable like a full-featured shell.

## Supported command forms

- **Single command:** `ls -l`
- **Sequential commands:** `date; ls; whoami`
- **Pipeline:** `ls | wc -l`
- **Input redirection:** `sort < names.txt`
- **Output redirection:** `echo hello > output.txt`
- **Append redirection:** `echo again >> output.txt`
- **Input and output redirection together:** the parser includes a path for combining `<` with `>` or `>>`.
- **Built-ins:** `chdir [directory]` and `quit`

### File descriptors, pipes, and redirection

For pipelines, `pipe(fd)` creates a kernel pipe with a read end and a write end. The child uses `dup2()` to connect a command's standard output to the pipe, or the next command's standard input to the pipe. Unneeded descriptors are closed with `close()`.

For redirection, `open()` obtains a file descriptor:

- `<` opens the input file with `O_RDONLY` and connects it to `STDIN_FILENO`.
- `>` opens or creates an output file and connects it to `STDOUT_FILENO`.
- `>>` opens or creates an output file with `O_APPEND` before connecting it to standard output.

The key operation is `dup2(file_descriptor, STDIN_FILENO/STDOUT_FILENO)`: it replaces the child's standard input or output with the selected file or pipe. Because redirection is performed in the forked child, the shell's own prompt and streams remain attached to the terminal.

## Build and run

This project uses POSIX APIs and is intended for Linux or another Unix-like environment. Build it with GCC and the included Makefile:

```bash
make
```

Start the shell with:

```bash
./shell
```

The Makefile compiles with `-ansi -pedantic -Wall`. Enter supported commands at the prompt; use `quit` or end input to leave.

## Files

- `shell.c` implements the prompt, command parsing, built-ins, child-process execution, pipelines, and redirection.
- `declarations.h` defines `BUFFER_SIZE` and declares the shell functions.
- `Makefile` builds the `shell` executable from `shell.c`.
- `.vscode/` contains editor and debugger configuration; it is not needed to compile or run the shell.

## Scope and limitations

This is an educational shell with a deliberately small parser, not a complete replacement for Bash or another POSIX shell:

- Arguments are split on spaces; quoting, escaped spaces, variable expansion, globbing, and command substitution are not implemented.
- External command paths are formed as `/bin/<name>` rather than resolved through `PATH`; commands installed elsewhere may not be found.
- The supported syntax is simple. Complex combinations of semicolons, pipelines, and redirections may not behave like they do in a standard shell.
- The pipeline implementation waits for a child while constructing the pipeline. Large outputs can fill a pipe before its consumer starts and cause the shell to stall.
- Output redirection does not specify `O_TRUNC`, so redirecting to an existing longer file may leave old trailing content. The source should be improved before relying on `>` for exact file replacement.
- Error handling and syntax validation cover only basic cases.

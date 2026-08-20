
# 🐚 minishell

A lightweight Unix shell built from scratch in C — a personal deep-dive into process management, signal handling, and inter-process communication at the systems level.

---

## ✨ Features

- 🔧 **Built-in commands** — `pwd`, `cd`, `echo` (partial), `exit`, `clear`
- 🕹️ **Job control** — `jobs`, `fg`, `bg` — stop with Ctrl-Z, resume in foreground or background
- 🔗 **Piping** — multi-stage pipelines (`cmd1 | cmd2 | cmd3`)
- 🛑 **Signal handling** — Ctrl-C / Ctrl-Z are forwarded to the active child instead of killing the shell
- 🎨 **Custom prompt** — change it live with `PS1=<name>`
- ✅ **Command recognition** — external commands matched against a whitelist (`extern.txt`) before `execvp`

---

## 🚀 Build & Run

```bash
git clone https://github.com/aaromal459/minishell.git
cd minishell
gcc main.c scaninput.c -o minishell
./minishell
```

---

## 🏗️ Architecture

| File | Responsibility |
|---|---|
| `main.c` | Entry point — signal setup for `SIGINT`, `SIGTSTP`, `SIGCHLD`; starts the REPL |
| `scaninput.c` | REPL loop, parsing, classification, execution, piping, job control |
| `main.h` | Shared types, constants, function declarations |
| `extern.txt` | Whitelist of recognized external commands |

### 🔄 How it works

```
main() sets up signal handlers, then calls scan_input() [runs forever]

  → read a line
  → get_command() extracts the first word
  → check_command_type() classifies it:
        BUILTIN     → execute_internal_commands()
        EXTERNAL    → fork + execvp (or pipe chain via execute_ext)
        NO_COMMAND  → "command not found"
```

Stopped jobs (Ctrl-Z) are tracked in a `stopped_process` linked list so `jobs` / `fg` / `bg` can manage them, and `sigchld_handler` cleans up the list when a tracked process fully exits.

---

## ⚠️ Known Limitations

This was built as a learning exercise, so it doesn't cover everything a production shell would:

- 🚫 No quoting/escaping (`echo "hello world"` won't parse as one argument)
- 🚫 No I/O redirection (`>`, `>>`, `<`) — only piping is implemented
- 🚫 `echo` doesn't do general `$VAR` expansion — only `$SHELL`, `$$`, `$?` are hardcoded
- 🚫 External commands must be explicitly listed in `extern.txt` rather than resolved via `$PATH`
- 🚫 No process groups (`setpgid`) — signal delivery to pipelines isn't fully isolated from the shell
- 🚫 Fixed-size buffers throughout (100 bytes input, 50 bytes/token) instead of dynamic allocation

---

## 🐞 Debugging Highlights

Two real bugs found and diagnosed while using the shell day-to-day:

**1. `PS1=` fallthrough bug**
Setting the prompt (`PS1=ARO`) succeeded, but was missing a `continue` on the success path — so the shell fell through and *also* tried to execute `PS1=ARO` as a command, printing `command not found` right after successfully changing the prompt.

**2. Lost job on Ctrl-Z (race condition)**
Without `SA_RESTART` on the `SIGINT`/`SIGTSTP` handler, a blocked `waitpid()` call was interrupted (`EINTR`) by signal delivery before `WIFSTOPPED` could be evaluated — so stopped jobs (e.g. `ping` after Ctrl-Z) silently vanished from tracking, even though the process was still alive in the background.

---

## 📚 What I Learned

- Process lifecycle management: `fork`, `execvp`, `wait`/`waitpid`
- Signal handling with `sigaction`, and why `SA_RESTART` matters for blocking syscalls
- Building pipelines manually with `pipe()` and `dup2()`
- Tracking shell job state with a linked list
- Diagnosing real race conditions between signal delivery and blocking system calls

---

## 👤 Author

**Aromal P R**
[GitHub](https://github.com/aaromal459)

---

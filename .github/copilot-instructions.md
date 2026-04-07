# Copilot instructions for `fuse-samples`

## Build, test, and lint commands

This repository is split into five standalone demos. Each demo has its own `Makefile` and binary target.

### Prerequisites

- `gcc`
- `pkg-config`
- `fuse3`

### Build

- Build one demo:
  - `cd 01-simple-demo && make`
  - `cd 02-trigger-demo && make`
  - `cd 03-multi-node-demo && make`
  - `cd 04-nested-node-demo && make`
  - `cd 05-extended-nested-node-demo && make`
- Build all demos:
  - `for d in 01-simple-demo 02-trigger-demo 03-multi-node-demo 04-nested-node-demo 05-extended-nested-node-demo; do (cd "$d" && make); done`

### Clean

- Clean one demo: `cd 03-multi-node-demo && make clean`
- Clean all demos:
  - `for d in 01-simple-demo 02-trigger-demo 03-multi-node-demo 04-nested-node-demo 05-extended-nested-node-demo; do (cd "$d" && make clean); done`

### Test / verification

There is no automated test suite target in this repository yet. Use mount-and-read checks per demo.

- Single-demo verification example (`02-trigger-demo`):
  1. `cd 02-trigger-demo && make`
  2. `mkdir -p /tmp/whoami-mnt`
  3. Run in one terminal: `./whoami_fuse /tmp/whoami-mnt -f`
  4. Run in another terminal: `cat /tmp/whoami-mnt/trigger.txt`

### Lint

There is no lint target/configuration in this repository.

## High-level architecture

- Root `README.md` is the index for the demos; each subdirectory is intentionally self-contained (source file + `Makefile` + README usage notes).
- The demos progress in complexity:
  1. `01-simple-demo`: static read-only file (`/hello.txt`) with fixed content.
  2. `02-trigger-demo`: one read-only file (`/trigger.txt`) whose read callback executes a shell command and returns output.
  3. `03-multi-node-demo`: multiple top-level files driven by a static node table and shared callback logic.
  4. `04-nested-node-demo`: nested directory tree with explicit directory/file lookup tables and path-aware output formatting.
  5. `05-extended-nested-node-demo`: extends the nested tree with `/mcu/version`; `/mcu/version` runs `ls /root` while `/mcu/sensor/*/version` runs `whoami` using `pipe` + `fork` + `execvp`.
- Every demo wires a `struct fuse_operations` with the same core callback set (`getattr`, `readdir`, `open`, `read`) and enters through `fuse_main(...)`.

## Key repository conventions

- Pin FUSE API level with `#define FUSE_USE_VERSION 31` in every demo.
- Keep files read-only (`0444`) and enforce read-only opens by rejecting non-`O_RDONLY` access with `-EACCES`.
- Unknown paths should consistently return `-ENOENT`.
- In larger demos, represent exposed filesystem entries as static arrays (`demo_nodes`, `demo_files`, `demo_dirs`) and resolve paths via helper lookups instead of hardcoding logic repeatedly.
- Keep command execution bounded and explicit. Existing demos use either a `popen` capture helper or `pipe` + `fork` + `execvp`; both paths should null-terminate buffers and surface failures as `-EIO` from read handlers.

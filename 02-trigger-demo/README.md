# Trigger Demo

This demo mounts a FUSE filesystem with one read-only file:

- `/trigger.txt`

Reading that file executes `whoami` and returns the command output as file content.

## Files

- `whoami_fuse.c`: FUSE implementation
- `Makefile`: local build and cleanup commands
- `whoami_fuse`: generated binary after `make`

## Build

```bash
make
```

## Clean

```bash
make clean
```

## Run

Create a mount point and start the filesystem:

```bash
mkdir -p /tmp/whoami-mnt
./whoami_fuse /tmp/whoami-mnt -f
```

In another terminal:

```bash
ls /tmp/whoami-mnt
cat /tmp/whoami-mnt/trigger.txt
```

Expected output format:

```text
Command: whoami
Output: <current-user>
```

## Behavior

- The root directory contains only `trigger.txt`.
- `trigger.txt` is read-only.
- Each read triggers a fresh `whoami` command execution.
- Any other path returns `ENOENT`.

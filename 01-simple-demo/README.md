# Simple Hello Demo

This demo mounts a minimal FUSE filesystem that exposes one read-only file:

- `/hello.txt`

Reading that file returns:

```text
Hello from FUSE on WSL2!
```

## Files

- `hello_fuse.c`: FUSE implementation
- `hello_fuse`: compiled binary

## Build

```bash
gcc hello_fuse.c -o hello_fuse $(pkg-config fuse3 --cflags --libs)
```

## Run

Create a mount point and start the filesystem:

```bash
mkdir -p /tmp/hello-mnt
./hello_fuse /tmp/hello-mnt -f
```

In another terminal:

```bash
ls /tmp/hello-mnt
cat /tmp/hello-mnt/hello.txt
```

## Behavior

- The root directory contains only `hello.txt`.
- `hello.txt` is read-only.
- Any other path returns `ENOENT`.

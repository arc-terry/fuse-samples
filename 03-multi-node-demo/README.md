# Multi-Node Demo

This demo mounts a read-only FUSE filesystem with three files:

- `/node-1.txt`
- `/node-2.txt`
- `/node-3.txt`

Reading any node runs `whoami` and appends node-specific information so you can tell which file triggered the callback.

## Files

- `multi_node_fuse.c`: FUSE implementation
- `Makefile`: local build and cleanup commands
- `multi_node_fuse`: generated binary after `make`

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
mkdir -p /tmp/multi-node-mnt
./multi_node_fuse /tmp/multi-node-mnt -f
```

In another terminal:

```bash
ls /tmp/multi-node-mnt
cat /tmp/multi-node-mnt/node-1.txt
cat /tmp/multi-node-mnt/node-2.txt
cat /tmp/multi-node-mnt/node-3.txt
```

Expected output format:

```text
Node: node-1
Path: /node-1.txt
Command: whoami
Output: <current-user>
```

## Behavior

- The root directory contains `node-1.txt`, `node-2.txt`, and `node-3.txt`.
- All nodes are read-only.
- Each read triggers a fresh `whoami` command execution.
- The response identifies the triggering node by name and path.
- Any other path returns `ENOENT`.

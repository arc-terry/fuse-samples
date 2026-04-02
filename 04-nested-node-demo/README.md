# Nested Node Demo

This demo mounts a read-only FUSE filesystem with nested folders and node files:

```text
/
  mcu/
    sensor/
      current/
        version
      thermal/
        version
      position/
        version
```

Reading any `version` node executes `whoami` and returns output that identifies which node triggered the callback.

## Files

- `nested_node_fuse.c`: FUSE implementation
- `Makefile`: local build and cleanup commands
- `nested_node_fuse`: generated binary after `make`

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
mkdir -p /tmp/nested-node-mnt
./nested_node_fuse /tmp/nested-node-mnt -f
```

In another terminal:

```bash
find /tmp/nested-node-mnt -maxdepth 4 -print
cat /tmp/nested-node-mnt/mcu/sensor/current/version
cat /tmp/nested-node-mnt/mcu/sensor/thermal/version
cat /tmp/nested-node-mnt/mcu/sensor/position/version
```

Expected output format:

```text
Node: mcu/sensor/current/version
Path: /mcu/sensor/current/version
Command: whoami
Output: <current-user>
```

## Behavior

- The filesystem tree matches the nested layout shown above.
- All `version` nodes are read-only files.
- Each read triggers a fresh `whoami` command execution.
- Output includes both node name and full path so the trigger is explicit.
- Any unknown path returns `ENOENT`.

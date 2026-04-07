# Extended Nested Node Demo

This demo mounts a read-only FUSE filesystem with nested folders and node files:

```text
/
  mcu/
    version
    sensor/
      current/
        version
      thermal/
        version
      position/
        version
```

Node command behavior:

- `/mcu/version` executes `ls /root`
- `/mcu/sensor/*/version` executes `whoami`

## Files

- `extended_nested_node_fuse.c`: FUSE implementation
- `Makefile`: local build and cleanup commands
- `extended_nested_node_fuse`: generated binary after `make`

## Build

```bash
make
```

## Clean

```bash
make clean
```

## Run

Create a mount point and start the filesystem.

If you mount with `sudo` and want non-root users to read the mount, use `-o allow_other` (not `allow_others`) and ensure `/etc/fuse.conf` contains `user_allow_other`:

```bash
mkdir -p /tmp/extended-nested-node-mnt
sudo ./extended_nested_node_fuse /tmp/extended-nested-node-mnt -f -o allow_other -o umask=022
```

The demo also injects safe defaults `max_threads=10` and `max_idle_threads=10` when they are not provided, avoiding invalid-thread warnings on some libfuse setups.

In another terminal:

```bash
find /tmp/extended-nested-node-mnt -maxdepth 4 -print
cat /tmp/extended-nested-node-mnt/mcu/version
cat /tmp/extended-nested-node-mnt/mcu/sensor/current/version
cat /tmp/extended-nested-node-mnt/mcu/sensor/thermal/version
cat /tmp/extended-nested-node-mnt/mcu/sensor/position/version
```

Expected output format:

```text
Node: mcu/version
Path: /mcu/version
Command: ls /root
Output: <contents-of-/root>
```

```text
Node: mcu/sensor/current/version
Path: /mcu/sensor/current/version
Command: whoami
Output: <current-user>
```

## Behavior

- The filesystem tree matches the nested layout shown above.
- All `version` nodes are read-only files.
- `/mcu/version` reads execute `ls /root`; sensor `version` reads execute `whoami`.
- Command execution uses `pipe` + `fork` + `execvp` (no `popen`).
- Output includes both node name and full path so the trigger is explicit.
- Any unknown path returns `ENOENT`.

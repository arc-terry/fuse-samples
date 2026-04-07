# FUSE Samples

This repository contains small FUSE3 demo projects written in C. Each folder is a standalone sample with its own source file, local `Makefile`, and usage notes.

## Projects

### `01-simple-demo`

A minimal read-only filesystem demo that exposes `/hello.txt` and returns a fixed greeting when read.

See [01-simple-demo/README.md](01-simple-demo/README.md) for build and run details.

### `02-trigger-demo`

A read-only filesystem demo that exposes `/trigger.txt` and runs `whoami` each time the file is read.

See [02-trigger-demo/README.md](02-trigger-demo/README.md) for build and run details.

### `03-multi-node-demo`

A read-only filesystem demo that exposes three files and runs `whoami` on each read, appending node-specific information so you can tell which path triggered the callback.

See [03-multi-node-demo/README.md](03-multi-node-demo/README.md) for build and run details.

### `04-nested-node-demo`

A read-only filesystem demo with nested folders under `/mcu/sensor`, where each `version` node runs `whoami` and reports which nested node was triggered.

See [04-nested-node-demo/README.md](04-nested-node-demo/README.md) for build and run details.

### `05-extended-nested-node-demo`

A read-only filesystem demo that extends the nested node layout with `/mcu/version`, executes `ls /root` for `/mcu/version`, and executes `whoami` for `/mcu/sensor/*/version` using `pipe` + `fork` + `execvp`.

See [05-extended-nested-node-demo/README.md](05-extended-nested-node-demo/README.md) for build and run details.

## Prerequisites

- `gcc`
- `pkg-config`
- `fuse3`

## Usage

Enter a project folder, build it with `make`, and remove generated artifacts with `make clean`. Use the README inside that folder for mount and runtime examples.

## Maintenance

Update this root README whenever a project's purpose, folder layout, build flow, or user-facing usage summary changes. Internal code-only changes do not require a root README edit unless they affect those details.

Overview of how the strai images are built:

Images without a "-cpu" or "-gpu" tag are built on ``ubuntu:22.04``. They are just an alias for **-cpu** (e.g. ``strai:latest`` is the same as ``strai:latest-cpu``).

```
ubuntu:22.04
└── base-deps:cpu
    └── strai:cpu

nvidia/cuda
└── base-deps:gpu
    └── strai:gpu
```

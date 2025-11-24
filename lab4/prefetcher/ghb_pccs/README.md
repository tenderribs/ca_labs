# Build Instructions

- Task 1 involves implementing an GHB-based PC/CS prefetcher using a constant prefetch degree. Task 2 uses the previous implementation in a scenario with limited bandwidth.
- Task 3 has an adaptively set prefetch degree based on the system's memory BW usage.

My implementation for the aforementioned tasks are in the same `ghb_pccs.h` and `ghb_pccs.cc` files. Supplying the `ADAPTIVE_PF_DEG` cpp build flag determines whether the prefetch degree is fixed (Task 1, 2) or dynamically adapted to the system's memory BW usage (Task 3).

To build the project for task 1, you should include a flag in the `make` command which sets the prefetch degree to a constant value.

```sh
# Prepare project directory
make clean
./config.sh dpc4/1C.CHANGE_ME.ghb_pccs.json

# for tasks 1 and 2
make -j

# for task 3
make CPPFLAGS=-DADAPTIVE_PF_DEG -j
```
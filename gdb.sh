gdb-multiarch -tui -ex "layout split" -ex "focus cmd" -ex "target extended-remote :3333" -x "modm/gdbinit" -x "modm/openocd_gdbinit" -ex "refresh" build/spincoater/cmake-build-release/spincoater.elf
q

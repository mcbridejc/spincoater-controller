gdb-multiarch -tui -ex "layout split" -ex "focus cmd" -ex "target extended-remote :3333" -x "modm/gdbinit" -x "modm/openocd_gdbinit" -ex "refresh" build/touchdemo-c++/cmake-build-release/spincoater.elf       :wq
q

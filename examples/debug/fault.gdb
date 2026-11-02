# Commands for tools/debug_qemu.sh: stop where the fault handler has filled fault_dump, decode it, and
# walk back to the line of the program that caused the fault.
set pagination off
target remote localhost:1234
source examples/debug/cmfault.py
break fault_startup.c:56
continue
print/x *frame@8
cmfault
backtrace
frame 2
info args

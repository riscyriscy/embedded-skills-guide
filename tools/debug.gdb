# Commands for tools/debug_qemu.sh. Stop in ringbuf_push, show the stack and registers, then run on.
set pagination off
target remote localhost:1234
break ringbuf_push
continue
backtrace
info args
info registers pc sp lr
x/3i $pc
delete
continue

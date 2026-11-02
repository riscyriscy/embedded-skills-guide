# gdb -batch -x crash.gdb ./lookup
set pagination off
run temperature humidity pressure
backtrace
info locals
print ch
up
info locals
print argv[i]
down
print ch->gain_permille

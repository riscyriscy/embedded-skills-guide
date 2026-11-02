# Stops only for the name that fails, shows what the function returns, and logs the calls without stopping.
set pagination off
dprintf calibrate,"calibrate name=%s raw=%d\n", name, raw
break find_channel if $_streq(name, "pressure")
run temperature humidity pressure
finish

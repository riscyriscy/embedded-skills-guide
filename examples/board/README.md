# examples/board

Programs for Chapter 15, section "From the simulator to a board". Nothing here needs a board.

    nodebug.c      a program that prints through semihosting, with a hard fault handler on the UART; run with semihosting on and off
    uart_time.c    SysTick counts for 80 characters through the UART of the simulator; run with -icount shift=0
    link_f407.ld   the project linker script for 1 MiB of flash at 0x08000000 and 128 KiB of RAM at 0x20000000
    baud_check.py  divisor, real baud rate and bit time for a clock, a baud rate and the clock the firmware assumed

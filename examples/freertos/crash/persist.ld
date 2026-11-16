/* The place of the crash record in RAM. Both linker scripts include this file, so the bootloader and
   the application cannot disagree about it. */
PERSIST_ORIGIN = 0x20000000;
PERSIST_SIZE   = 0x400;

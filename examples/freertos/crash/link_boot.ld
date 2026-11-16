ENTRY(Reset_Handler)
INCLUDE persist.ld

MEMORY
{
    FLASH   (rx)  : ORIGIN = 0x00000000, LENGTH = 32K
    PERSIST (rw)  : ORIGIN = PERSIST_ORIGIN, LENGTH = PERSIST_SIZE
    RAM     (rwx) : ORIGIN = PERSIST_ORIGIN + PERSIST_SIZE, LENGTH = 128K - PERSIST_SIZE
}

_estack = ORIGIN(RAM) + LENGTH(RAM);
_flash_begin = ORIGIN(FLASH);
_flash_end = ORIGIN(FLASH) + LENGTH(FLASH);
_ram_begin = ORIGIN(PERSIST);
_ram_end = ORIGIN(RAM) + LENGTH(RAM);

SECTIONS
{
    .isr_vector : { KEEP(*(.isr_vector)) } > FLASH
    .text       : { *(.text*) *(.rodata*) . = ALIGN(4); } > FLASH

    .persist (NOLOAD) : { *(.persist*) } > PERSIST /* not in the image, not cleared at start-up */

    _sidata = LOADADDR(.data);
    .data : { _sdata = .; *(.data*) . = ALIGN(4); _edata = .; } > RAM AT > FLASH
    .bss  : { _sbss = .; *(.bss*) *(COMMON) . = ALIGN(4); _ebss = .; } > RAM
}

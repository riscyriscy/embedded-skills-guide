/* A linker script for the secure image of the TrustZone example. The secure gateway
   veneers (sg instructions) are placed by the option
   --section-start=.gnu.sgstubs=<address> of the link command. */
MEMORY
{
    FLASH (rx) : ORIGIN = 0x10000000, LENGTH = 252K
}
SECTIONS
{
    .text : { *(.text*) *(.rodata*) } > FLASH
}

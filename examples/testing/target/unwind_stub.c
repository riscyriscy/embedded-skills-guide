/* The setjmp of newlib refers to the unwinder of the ARM exception ABI, and the unwinder in turn
   needs the sections .ARM.exidx and .ARM.extab, which the linker script of this project does not
   have. The tests never unwind a stack, so an empty function satisfies the reference and keeps
   the unwinder out of the image. */
void __aeabi_unwind_cpp_pr0(void)
{
}

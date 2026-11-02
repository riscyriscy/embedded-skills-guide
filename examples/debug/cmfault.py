"""A gdb command for Cortex-M faults. In gdb: source cmfault.py, then run cmfault when the program has
stopped in fault_report. It reads the variable fault_dump that the fault handler filled, names every
bit that is set in the configurable fault status register (CFSR) and the hard fault status register
(HFSR), and shows the source line of the instruction that faulted."""
import gdb

CFSR_BITS = {
    0: "IACCVIOL: instruction fetch from a region that is not allowed",
    1: "DACCVIOL: data access to a region that is not allowed",
    3: "MUNSTKERR: fault on return from exception, unstacking",
    4: "MSTKERR: fault on entry to exception, stacking",
    7: "MMARVALID: MMFAR holds the faulting address",
    8: "IBUSERR: bus error on instruction fetch",
    9: "PRECISERR: precise data bus error, the address is exact",
    10: "IMPRECISERR: imprecise data bus error, the pc is later than the access",
    11: "UNSTKERR: bus error on unstacking",
    12: "STKERR: bus error on stacking",
    15: "BFARVALID: BFAR holds the faulting address",
    16: "UNDEFINSTR: undefined instruction",
    17: "INVSTATE: instruction set state not allowed (Arm bit set, no Thumb bit)",
    18: "INVPC: invalid exception return value",
    19: "NOCP: coprocessor not available",
    24: "UNALIGNED: unaligned access that traps",
    25: "DIVBYZERO: integer division by zero",
}
HFSR_BITS = {
    1: "VECTTBL: bus error reading the vector table",
    30: "FORCED: escalated from a configurable fault",
    31: "DEBUGEVT: debug event",
}


class CmFault(gdb.Command):
    """Decode the fault registers saved in fault_dump."""

    def __init__(self):
        super().__init__("cmfault", gdb.COMMAND_USER)

    def invoke(self, arg, from_tty):
        dump = gdb.parse_and_eval("fault_dump")
        regs = {f.name: int(dump[f.name]) & 0xFFFFFFFF for f in dump.type.fields()}
        if not regs["valid"]:
            gdb.write("fault_dump is empty: the program has not faulted\n")
            return
        for name in ("r0", "r1", "r2", "r3", "r12", "lr", "pc", "psr"):
            gdb.write("%-5s 0x%08x\n" % (name, regs[name]))
        for label, key, table in (("CFSR", "cfsr", CFSR_BITS), ("HFSR", "hfsr", HFSR_BITS)):
            gdb.write("%s  0x%08x\n" % (label, regs[key]))
            for bit in sorted(table):
                if regs[key] >> bit & 1:
                    gdb.write("  bit %2d  %s\n" % (bit, table[bit]))
        if regs["cfsr"] >> 15 & 1:
            gdb.write("BFAR  0x%08x\n" % regs["bfar"])
        if regs["cfsr"] >> 7 & 1:
            gdb.write("MMFAR 0x%08x\n" % regs["mmfar"])
        gdb.write("faulting instruction:\n")
        gdb.execute("info symbol 0x%x" % regs["pc"])
        gdb.execute("info line *0x%x" % regs["pc"])
        gdb.write("called from (lr): ")
        gdb.execute("info symbol 0x%x" % (regs["lr"] & ~1))


CmFault()

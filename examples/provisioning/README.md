# examples/provisioning

Programs for Chapter 11 of Volume 2. They run under QEMU (machine mps2-an386) and need `openssl` and Python 3.

| File | Purpose |
|---|---|
| `unit_record.h`, `unit_record.c` | the 52-byte record of one unit, its CRC-32 and the check that the firmware makes |
| `prov_demo.c` | reads the record, prints the unit and applies the calibration of that unit |
| `link_prov.ld` | the project linker script with 508 KiB of flash and the record at 0x0007F000 |
| `provision.py` | the factory tool: `make` writes a record and a line in the log, `show` reads a record back |
| `run.sh` | one image, an erased unit, two units that the factory wrote and one damaged record |

Run:

```
examples/provisioning/run.sh /tmp/prov
```

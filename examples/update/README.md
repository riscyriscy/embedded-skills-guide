# examples/update

Programs for Chapter 28.

    ab_sim.py       a device with two slots, a boot counter and a rollback; "selftest" cuts the power at every step
    make_bundle.sh  a RAUC bundle for a root file system image, with a test CA and a test signing certificate

The release itself is made by `tools/release.sh` and checked by `tools/verify_release.sh`.

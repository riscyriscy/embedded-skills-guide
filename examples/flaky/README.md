# examples/flaky

A test that fails in some runs and passes in the others, for Chapter 8 of Volume 2. It needs CMake, Ninja and a C compiler.

| File | Purpose |
|---|---|
| `tally_test.c` | two threads add to a tally without a lock; `--locked` takes a mutex |
| `CMakeLists.txt` | the two tests, with the label `quarantine` on the unlocked one |
| `CMakePresets.json` | the test presets `gate`, which leaves the label out, and `quarantine`, which runs only that label |

Run:

```
cd examples/flaky
cmake --preset host && cmake --build --preset host
ctest --preset gate
../../tools/repeat.sh 200 ../../build/flaky/tally_test
```

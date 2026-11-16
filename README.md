# sensorfw: companion code for the Embedded Skills Guide

Source code of the running project of "Embedded Skills Guide" (C. Paul Gordon, Embedded Think, November 2026).
Every chapter of the book builds, tests, formats, debugs or ships this project, and every listing in the book
comes from this repository.

`sensorfw` is small on purpose. A ring buffer keeps the newest samples of a sensor, and a library computes their
mean and median. A platform layer hides the differences between the build machine and a Cortex-M4. A demo
program, a unit test and the scripts of the book complete it.

## Quick start

```
git clone https://github.com/riscyriscy/embedded-skills-guide
cd embedded-skills-guide
tools/check_env.sh        # lists the tools that are missing
tools/ci.sh all           # host build, tests, target build under QEMU, static checks
```

`tools/ci.sh` is the one script that GitHub Actions (`.github/`), GitLab CI (`.gitlab-ci.yml`) and Jenkins
(`Jenkinsfile`) call, so a stage that fails on the server fails the same way on your machine. A single stage
runs alone, for example `tools/ci.sh m4`. The tools that each stage needs and their versions are in the book,
Chapter 1, and in `docker/`.

## Layout

| Path | Contents |
|---|---|
| `lib/`, `lib/filters/` | The `sensor` library (ring buffer, mean, version stamp) and the `filters` library (median of three) |
| `platform/` | `platform.h`, a `host` implementation and an `arm-m4` implementation with startup code and a linker script |
| `app/`, `test/` | `sensor_demo` and the unit test `test_sensor` |
| `cmake/`, `CMakeLists.txt`, `CMakePresets.json`, `Makefile` | The CMake build with a toolchain file and presets, and the same build written by hand |
| `docker/` | The build image with pinned tools and the image of the pipeline |
| `tools/`, `tools/sfw/` | Scripts for checks, size reports, debugging, the release and the documentation build, and a Python dashboard of pipeline reports |
| `docs/`, `mkdocs.yml` | The documentation site and the Doxygen settings |
| `examples/` | Programs for the language, testing, concurrency, Linux, debugging, update and documentation chapters; the project build does not use them. `examples/run.sh` builds and runs one of them for the host or the Cortex-M4 |
| `.clang-format`, `.clang-tidy`, `.clangd`, `.editorconfig`, `.vscode/`, `.githooks/` | Editor, formatter, analyzer and hook settings |

The history has roughly one commit for each part of the book, from the Part 1 tooling to the Linux hardware examples, so `git log` shows the project as it stood after each part. The tag `v0.1.0` marks the first commit.

## FreeRTOS and Zephyr examples

The programs under `examples/freertos/` are a larger set than Chapter 18 explains. Chapter 18 is a working tour of the kernel. The full set is explained in The Embedded FreeRTOS Handbook (https://github.com/riscyriscy/freertos-handbook): critical sections, the deferral of interrupt work, notifications, queues, measurement, timers, stream buffers, static allocation and the crash record. The build and the tests of this repository still cover every program.

`examples/zephyr/app-manifest/west.yml` is the manifest of an application repository that imports Zephyr 4.5.0 with three modules. Chapter 19 shows how to use it: copy the directory into a git repository of its own and run `west init -l` on it.

## Third-party code

`examples/freertos/FreeRTOS-Kernel/` holds the files of FreeRTOS-Kernel V11.3.3 that the examples build, under the
MIT licence of that project (`LICENSE.md` in the same directory). `examples/testing/unity/` holds the Unity test
framework under its MIT licence (`LICENSE.txt`). Everything else is under the licence in `LICENSE`.

## Licence

MIT. See `LICENSE`.

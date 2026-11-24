# Review checklist for firmware changes

Every item is a check that a reviewer can answer from the diff, the test output or the map file.

## Behaviour

- [ ] The change has a test, or the description says why it cannot have one.
- [ ] Every new path that can fail returns or reports the failure, and the caller handles it.
- [ ] Preconditions that only a programming mistake can break use REQUIRE, not an error code.
- [ ] A loop that waits for hardware has a time limit.

## Interrupts and concurrency

- [ ] Data shared between an interrupt and the main loop is volatile or atomic, and a multi word value is read under a lock.
- [ ] An interrupt handler does bounded work: no blocking call, no loop that waits for hardware, no printf.
- [ ] The watchdog is fed by one place that checks that every task made progress.

## Memory

- [ ] Buffers have a size that the code checks, and the size is a named constant.
- [ ] No heap allocation after start-up, or the allocation is bounded and its failure is handled.
- [ ] The stack use of new functions is visible in the .su files, and deep recursion is absent.
- [ ] The change in flash and RAM from tools/size_report.sh is in the description when it is above 1 percent.

## Interfaces and releases

- [ ] A change to a stored format, a wire format or a flash layout has a version number and a migration for the old data.
- [ ] The commit message starts with a type (feat, fix, perf, docs, test, build, refactor, chore), and a breaking change has "!" after it.
- [ ] A requirement that the change touches is named in the test (REQ-nnn), and tools/trace.py still reports every requirement covered.
- [ ] No secret, key or password is in the diff, and tools/secret_scan.sh reports no finding.

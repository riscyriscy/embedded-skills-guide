#!/bin/bash
# Runs the static checks of the project and prints one line per check.
# The log of each check goes to build/logs. Exit status 1 when any check fails.
# The checks are called through "$check", which ShellCheck cannot follow.
# shellcheck disable=SC2317
set -u
cd "$(git rev-parse --show-toplevel)" || exit 1
mkdir -p build/logs

# Warnings that the build treats as errors. The sources of the target are checked with the target compiler.
strict=(-std=c17 -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion -Wdouble-promotion
        -Wformat=2 -Wundef -Wcast-align -Wcast-qual -Wstrict-prototypes -Wmissing-prototypes
        -Wmissing-declarations -Wredundant-decls -Wold-style-definition -Wvla -Wswitch-enum
        -Wnull-dereference -Wduplicated-cond -Wlogical-op -Werror)
includes=(-DRINGBUF_CAPACITY=16u -Ilib/include -Ilib/filters/include -Iplatform/include)
# Start-up code is allowed two relaxations: the reset handler is only called from the vector table,
# and the vector table converts a function pointer to an object pointer.
relaxed=(-Wno-missing-prototypes -Wno-missing-declarations -Wno-pedantic)

warnings() {
    local f status=0 extra
    while read -r f; do
        case $f in
            platform/arm-m4/*)
                extra=()
                [ "$f" = platform/arm-m4/startup.c ] && extra=("${relaxed[@]}")
                arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -Os "${strict[@]}" "${includes[@]}" "${extra[@]}" -fsyntax-only "$f" || status=1 ;;
            *)
                gcc -O2 "${strict[@]}" "${includes[@]}" -fsyntax-only "$f" || status=1 ;;
        esac
    done < <(git ls-files 'lib/*.c' 'app/*.c' 'platform/*.c' 'test/*.c')
    return "$status"
}

format() {
    # The sources of the project, as in warnings and tidy; the examples are laid out for the book.
    git ls-files 'lib/*.c' 'lib/*.h' 'app/*.c' 'platform/*.c' 'platform/*.h' 'test/*.c' |
        xargs clang-format --dry-run --Werror
}

tidy() {
    [ -f build/host/compile_commands.json ] || cmake --preset host > /dev/null
    local files
    mapfile -t files < <(git ls-files 'lib/*.c' 'app/*.c' 'platform/host/*.c' 'test/*.c')
    clang-tidy -p build/host --quiet "${files[@]}"
}

cppcheck_step() {
    cppcheck --enable=warning,style,performance,portability --inline-suppr \
        --suppressions-list=cppcheck-suppressions.txt --error-exitcode=1 -q \
        "${includes[@]}" lib app platform test
}

scripts() {
    shellcheck tools/*.sh .githooks/pre-commit examples/*.sh examples/*/*.sh
}

failed=0
for check in warnings format tidy cppcheck_step scripts; do
    name=${check%_step}
    if $check > "build/logs/check-$name.log" 2>&1; then
        printf '%-10s ok\n' "$name"
    else
        printf '%-10s FAILED  see build/logs/check-%s.log\n' "$name" "$name"
        failed=1
    fi
done
exit $failed

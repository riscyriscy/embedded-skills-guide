#!/bin/bash
# The steps of the pipeline. GitHub Actions, GitLab CI and Jenkins call this script and nothing else,
# so a developer runs the same commands on a laptop. One line per stage, the log of each stage goes to
# build/logs and the reports (JUnit files, coverage, sizes, version) go to build/reports.
# Usage: tools/ci.sh stage [stage ...]      stages: env host asan coverage m4 static board meta publish all
# With VERBOSE=1 the output of each stage also goes to the console, where a pipeline keeps it.
# Exit status 1 when any stage fails. Every stage runs, so one run shows every failure.
# The stages are called through "stage_$name", which ShellCheck cannot follow.
# shellcheck disable=SC2317
set -u
cd "$(git rev-parse --show-toplevel)" || exit 1
root=$PWD
logs=$root/build/logs
reports=$root/build/reports
mkdir -p "$logs" "$reports"

# Coverage gates. Raise them when the figures rise.
: "${COV_LINE:=90}" "${COV_BRANCH:=90}"
# Limits of the part for the size report: 256 KiB of flash and 64 KiB of RAM.
: "${FLASH_LIMIT:=262144}" "${RAM_LIMIT:=65536}"

stage_env() { tools/check_env.sh; }

stage_host() {
    cd "$root/examples/testing" || return 1
    cmake --preset host && cmake --build --preset host &&
        ctest --preset host --output-junit "$reports/junit-host.xml"
}

stage_asan() {
    cd "$root/examples/testing" || return 1
    cmake --preset host-asan && cmake --build --preset host-asan &&
        ctest --preset host-asan --output-junit "$reports/junit-asan.xml"
}

stage_coverage() {
    cd "$root/examples/testing" || return 1
    cmake --preset host-coverage && cmake --build --preset host-coverage &&
        ctest --preset host-coverage --output-junit "$reports/junit-coverage.xml" || return 1
    # The reports are written first, the gates are checked by the second run, so a failed gate still leaves a report.
    local g=(gcovr --root ../.. --object-directory ../../build/testing-host-coverage
             --filter ../../lib/ --filter frame/ --filter sampler/)
    "${g[@]}" --json-summary "$reports/coverage.json" --json-summary-pretty --cobertura "$reports/coverage.xml" > /dev/null || return 1
    "${g[@]}" --fail-under-line "$COV_LINE" --fail-under-branch "$COV_BRANCH"
}

stage_m4() {
    cd "$root/examples/testing" || return 1
    cmake --preset m4 && cmake --build --preset m4 &&
        ctest --preset m4 --output-junit "$reports/junit-m4.xml" || return 1
    cd "$root" || return 1
    cmake --workflow --preset ci-m4 || return 1
    local elf=build/m4-release/app/sensor_demo
    tools/size_report.sh "$elf" "$FLASH_LIMIT" "$RAM_LIMIT" || return 1
    arm-none-eabi-size "$elf" | awk -v fl="$FLASH_LIMIT" -v rl="$RAM_LIMIT" \
        'NR == 2 { printf "{\"flash\": %d, \"ram\": %d, \"flash_limit\": %d, \"ram_limit\": %d}\n", $1 + $2, $2 + $3, fl, rl }' \
        > "$reports/size.json"
}

stage_static() { tools/static_check.sh; }

# The tests on a board that is connected to the machine of the pipeline. BOARD_TTY names its serial port.
# This stage is not part of "all": a runner without the board cannot run it.
stage_board() {
    : "${BOARD_TTY:?BOARD_TTY is not set: the stage needs the serial port of the board}"
    python3 examples/testing/target_test.py --device "$BOARD_TTY" --baud 115200 --junit "$reports/junit-board.xml"
}

# The identity of the run: what the version stamp says, which commit and when.
stage_meta() {
    local describe commit subject
    describe=$(git describe --tags --always --dirty)
    commit=$(git rev-parse HEAD)
    subject=$(git log -1 --format=%s)
    subject=${subject//\"/}    # no quotes or backslashes, so the JSON stays valid
    subject=${subject//\\/}
    printf '{"version": "%s", "commit": "%s", "subject": "%s", "time": "%s"}\n' \
        "$describe" "$commit" "$subject" "$(date -u +%Y-%m-%dT%H:%M:%SZ)" > "$reports/meta.json"
}

# Sends the reports of the run to the dashboard. DASHBOARD_URL is the address, SFW_TOKEN comes from the secret store
# of the pipeline. This stage is not part of "all": a developer's laptop does not publish.
stage_publish() {
    : "${DASHBOARD_URL:?DASHBOARD_URL is not set: the stage needs the address of the dashboard}"
    : "${SFW_TOKEN:?SFW_TOKEN is not set: the stage needs the token of the dashboard}"
    uv run --project tools/sfw --locked --no-dev sfw publish "$reports" --url "$DASHBOARD_URL"
}

run_stage() {
    local name=$1 start status
    start=$(date +%s)
    if [ -n "${VERBOSE:-}" ]; then
        ( "stage_$name" ) 2>&1 | tee "$logs/$name.log"
        status=${PIPESTATUS[0]}
    else
        ( "stage_$name" ) > "$logs/$name.log" 2>&1
        status=$?
    fi
    local result seconds
    result=$([ "$status" -eq 0 ] && echo passed || echo failed)
    seconds=$(( $(date +%s) - start ))
    printf '%-9s %-7s %3d s   build/logs/%s.log\n' "$name" "${result/failed/FAILED}" "$seconds" "$name"
    # The result of the stage is a report like the others, so a dashboard knows about a failed gate as well as failed tests.
    printf '{"name": "%s", "status": "%s", "seconds": %d}\n' "$name" "$result" "$seconds" > "$reports/stage-$name.json"
    if [ "$status" -ne 0 ]; then tail -n 4 "$logs/$name.log" | sed 's/^/    /'; fi
    return "$status"
}

stages=("$@")
[ ${#stages[@]} -gt 0 ] || { echo "usage: tools/ci.sh stage [stage ...]   (env host asan coverage m4 static board meta publish all)" >&2; exit 2; }
[ "${stages[0]}" = all ] && stages=(env static host asan coverage m4 meta)

failed=0
for s in "${stages[@]}"; do
    if ! declare -F "stage_$s" > /dev/null; then echo "ci.sh: unknown stage: $s" >&2; exit 2; fi
    run_stage "$s" || failed=1
done
exit $failed

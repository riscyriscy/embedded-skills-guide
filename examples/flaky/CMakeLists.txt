cmake_minimum_required(VERSION 3.25)
project(flaky_demo LANGUAGES C)

enable_testing()
find_package(Threads REQUIRED)

add_executable(tally_test tally_test.c)
target_link_libraries(tally_test PRIVATE Threads::Threads)

add_test(NAME tally_unlocked COMMAND tally_test)
add_test(NAME tally_locked COMMAND tally_test --locked)

# A test that fails now and then waits here until someone repairs it. The "gate" preset leaves the label
# out, so the test does not stop a merge, and the "quarantine" preset still runs it and shows the result.
set_tests_properties(tally_unlocked PROPERTIES LABELS quarantine)

# Run in script mode at build time:
#   cmake -DSRC=<in> -DDST=<out> -DPROJECT_VERSION=<x.y.z> -DGIT_DIR=<dir> -P GenVersion.cmake
execute_process(
    COMMAND git describe --tags --always --dirty
    WORKING_DIRECTORY "${GIT_DIR}"
    OUTPUT_VARIABLE GIT_DESCRIBE
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET)
if(NOT GIT_DESCRIBE)
    set(GIT_DESCRIBE "no-git")
endif()
# configure_file rewrites the output only when the content changes, so an unchanged
# version does not trigger a recompile of version.c.
configure_file("${SRC}" "${DST}" @ONLY)

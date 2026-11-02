# This file is read when lib/CMakeLists.txt calls add_subdirectory(filters).
# Relative paths below are relative to lib/filters, the current source directory.
add_library(filters STATIC src/median.c)
target_include_directories(filters PUBLIC include)

# filter/median.h includes sensor/ringbuf.h, so the users of filters need sensor too: PUBLIC.
target_link_libraries(filters PUBLIC sensor PRIVATE project_warnings)

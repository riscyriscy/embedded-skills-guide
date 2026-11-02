add_library(platform STATIC)
target_include_directories(platform PUBLIC include)
target_link_libraries(platform PRIVATE project_warnings)

if(CMAKE_CROSSCOMPILING)
    target_sources(platform PRIVATE arm-m4/platform_semihost.c)

    # Startup code is an OBJECT library: its vector table is not referenced by any other
    # object, so inside a static archive the linker would never pull it in.
    add_library(startup OBJECT arm-m4/startup.c)
    target_link_libraries(startup PRIVATE project_warnings)

    add_library(firmware_link INTERFACE)
    target_link_options(firmware_link INTERFACE -T${CMAKE_CURRENT_SOURCE_DIR}/arm-m4/link.ld)
    set_property(TARGET firmware_link APPEND PROPERTY INTERFACE_LINK_DEPENDS
                 ${CMAKE_CURRENT_SOURCE_DIR}/arm-m4/link.ld)
else()
    target_sources(platform PRIVATE host/platform_host.c)
    add_library(firmware_link INTERFACE)
endif()

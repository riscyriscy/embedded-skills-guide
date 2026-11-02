# firmware_finish(<target>): map file always; .bin and a size report on cross builds.
function(firmware_finish tgt)
    target_link_options(${tgt} PRIVATE "-Wl,-Map=$<TARGET_FILE:${tgt}>.map")
    if(CMAKE_CROSSCOMPILING)
        add_custom_command(TARGET ${tgt} POST_BUILD
            COMMAND arm-none-eabi-objcopy -O binary $<TARGET_FILE:${tgt}> $<TARGET_FILE:${tgt}>.bin
            COMMAND arm-none-eabi-size $<TARGET_FILE:${tgt}>
            VERBATIM)
    endif()
endfunction()

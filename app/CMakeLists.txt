add_executable(sensor_demo main.c)
target_link_libraries(sensor_demo PRIVATE sensor platform firmware_link project_warnings)
firmware_finish(sensor_demo)

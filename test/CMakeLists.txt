add_executable(test_sensor test_sensor.c)
target_link_libraries(test_sensor PRIVATE sensor platform firmware_link project_warnings)
firmware_finish(test_sensor)

add_test(NAME sensor_unit COMMAND test_sensor)
add_test(NAME sensor_demo_runs COMMAND sensor_demo)
set_tests_properties(sensor_demo_runs PROPERTIES PASS_REGULAR_EXPRESSION "mean ok")

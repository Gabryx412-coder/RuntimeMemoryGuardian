# ==============================================================================
# Runtime Memory Guardian - Example Applications
# ==============================================================================

function(rmg_add_example example_name)
    add_executable(${example_name} ${ARGN})
    target_link_libraries(${example_name} PRIVATE RMG::rmg)
    rmg_enable_warnings(${example_name})
    rmg_enable_sanitizers(${example_name})
    set_target_properties(${example_name} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/examples"
    )
endfunction()

rmg_add_example(01_basic_integrity_check 01_basic_integrity_check/main.cpp)
rmg_add_example(02_memory_scanning      02_memory_scanning/main.cpp)
rmg_add_example(03_hook_detection       03_hook_detection/main.cpp)
rmg_add_example(04_module_monitoring    04_module_monitoring/main.cpp)
rmg_add_example(05_full_process_monitor 05_full_process_monitor/main.cpp)
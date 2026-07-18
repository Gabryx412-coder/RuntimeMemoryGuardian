# ==============================================================================
# Runtime Memory Guardian - Command-Line Tools
# ==============================================================================

add_executable(rmg-cli rmg-cli/main.cpp)
target_link_libraries(rmg-cli PRIVATE RMG::rmg)
rmg_enable_warnings(rmg-cli)
rmg_enable_sanitizers(rmg-cli)
set_target_properties(rmg-cli PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tools"
)

add_executable(rmg-baseline-generator baseline-generator/main.cpp)
target_link_libraries(rmg-baseline-generator PRIVATE RMG::rmg)
rmg_enable_warnings(rmg-baseline-generator)
rmg_enable_sanitizers(rmg-baseline-generator)
set_target_properties(rmg-baseline-generator PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tools"
)
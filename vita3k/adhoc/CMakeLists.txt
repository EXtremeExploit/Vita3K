add_library(
    adhoc
    STATIC
    src/state.cpp
)

target_include_directories(adhoc PUBLIC include)
target_link_libraries(adhoc PRIVATE util kernel net emuenv kernel)
target_link_libraries(adhoc PUBLIC modules module)

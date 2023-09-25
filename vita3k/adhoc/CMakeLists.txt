add_library(
    adhoc
    STATIC
    src/state.cpp
    src/matchingContext.cpp
    src/threads.cpp
)

target_include_directories(adhoc PUBLIC include)
target_link_libraries(adhoc PRIVATE util kernel net emuenv kernel io)
target_link_libraries(adhoc PUBLIC modules module)

add_library(lib
    task_pool.cpp
    scheduler.cpp
)

target_include_directories(lib
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)
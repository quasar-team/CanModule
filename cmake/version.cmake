execute_process(
    COMMAND git rev-parse --short HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_COMMIT_HASH
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)

execute_process(
    COMMAND git log -1 --format=%cd
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_COMMIT_DATE
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)

if(NOT GIT_COMMIT_HASH)
    set(GIT_COMMIT_HASH "unknown")
endif()

if(NOT GIT_COMMIT_DATE)
    set(GIT_COMMIT_DATE "unknown")
endif()

configure_file (
    "${PROJECT_SOURCE_DIR}/src/include/CanVersion.h.in"
    "${PROJECT_BINARY_DIR}/src/include/CanVersion.h"
    @ONLY
)

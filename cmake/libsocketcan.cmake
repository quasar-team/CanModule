include(FetchContent)

set(LIBSOCKETCAN_DEFAULT_URL "https://github.com/linux-can/libsocketcan/archive/refs/tags/v0.0.12.zip")
set(LIBSOCKETCAN_DEFAULT_HASH "SHA256=e634d623990f63b4d67beb5a40daf5db73fb37d1a3b3acf68265b60f3f389fbf")

set(LIBSOCKETCAN_URL "${LIBSOCKETCAN_DEFAULT_URL}" CACHE STRING "URL used to download libsocketcan")
set(LIBSOCKETCAN_HASH "" CACHE STRING "Optional checksum for a custom libsocketcan URL")

set(LIBSOCKETCAN_FETCHCONTENT_ARGS
    URL "${LIBSOCKETCAN_URL}"
    DOWNLOAD_EXTRACT_TIMESTAMP True
)

if(LIBSOCKETCAN_URL STREQUAL LIBSOCKETCAN_DEFAULT_URL)
    set(_libsocketcan_hash "${LIBSOCKETCAN_HASH}")
    if(_libsocketcan_hash STREQUAL "")
        set(_libsocketcan_hash "${LIBSOCKETCAN_DEFAULT_HASH}")
    endif()
    list(APPEND LIBSOCKETCAN_FETCHCONTENT_ARGS
        URL_HASH "${_libsocketcan_hash}"
    )
elseif(NOT LIBSOCKETCAN_HASH STREQUAL "")
    list(APPEND LIBSOCKETCAN_FETCHCONTENT_ARGS
        URL_HASH "${LIBSOCKETCAN_HASH}"
    )
else()
    message(STATUS "Downloading libsocketcan without checksum validation: ${LIBSOCKETCAN_URL}")
endif()

FetchContent_Declare(
    libsocketcan
    ${LIBSOCKETCAN_FETCHCONTENT_ARGS}
)

FetchContent_MakeAvailable(libsocketcan)

set(LIBSOCKETCAN_INCLUDE_DIR ${libsocketcan_SOURCE_DIR}/include)

set(LIBSOCKETCAN_LIB ${libsocketcan_SOURCE_DIR}/src/.libs/libsocketcan.a)

add_custom_command(
    OUTPUT ${LIBSOCKETCAN_LIB}
    WORKING_DIRECTORY ${libsocketcan_SOURCE_DIR}
    COMMAND bash autogen.sh
    COMMAND bash configure --enable-static --disable-shared CFLAGS=-fPIC CXXFLAGS=-fPIC
    COMMAND make
    DEPENDS ${libsocketcan_SOURCE_DIR}
)

add_library(libsocketcan STATIC IMPORTED GLOBAL)

set_target_properties(libsocketcan PROPERTIES
    IMPORTED_LOCATION ${LIBSOCKETCAN_LIB}
    INTERFACE_INCLUDE_DIRECTORIES ${LIBSOCKETCAN_INCLUDE_DIR}
)

add_custom_target(libsocketcan_build ALL
    DEPENDS ${LIBSOCKETCAN_LIB}
)

add_dependencies(libsocketcan libsocketcan_build)

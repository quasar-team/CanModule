include(ExternalProject)

set(LIBSOCKETCAN_DEFAULT_URL
    "https://github.com/linux-can/libsocketcan/archive/refs/tags/v0.0.12.zip"
)

set(LIBSOCKETCAN_DEFAULT_HASH
    "SHA256=e634d623990f63b4d67beb5a40daf5db73fb37d1a3b3acf68265b60f3f389fbf"
)

set(LIBSOCKETCAN_URL "${LIBSOCKETCAN_DEFAULT_URL}"
    CACHE STRING "URL used to download libsocketcan"
)

set(LIBSOCKETCAN_HASH ""
    CACHE STRING "Checksum for libsocketcan. Required for custom URLs unless explicitly disabled."
)

option(LIBSOCKETCAN_ALLOW_UNVERIFIED_DOWNLOAD
    "Allow downloading libsocketcan without checksum validation"
    OFF
)

set(_libsocketcan_url_hash)

if(LIBSOCKETCAN_HASH)
    set(_libsocketcan_url_hash URL_HASH "${LIBSOCKETCAN_HASH}")
elseif(LIBSOCKETCAN_URL STREQUAL LIBSOCKETCAN_DEFAULT_URL)
    set(_libsocketcan_url_hash URL_HASH "${LIBSOCKETCAN_DEFAULT_HASH}")
elseif(NOT LIBSOCKETCAN_ALLOW_UNVERIFIED_DOWNLOAD)
    message(FATAL_ERROR
        "LIBSOCKETCAN_HASH must be set when using a custom LIBSOCKETCAN_URL. "
        "Set LIBSOCKETCAN_ALLOW_UNVERIFIED_DOWNLOAD=ON to bypass this check."
    )
else()
    message(WARNING
        "Downloading libsocketcan without checksum validation: ${LIBSOCKETCAN_URL}"
    )
endif()

find_program(SH_EXE NAMES sh bash REQUIRED)
find_program(MAKE_EXE NAMES gmake make REQUIRED)

set(LIBSOCKETCAN_PREFIX_DIR  "${CMAKE_BINARY_DIR}/_deps/libsocketcan")
set(LIBSOCKETCAN_SOURCE_DIR  "${LIBSOCKETCAN_PREFIX_DIR}/src")
set(LIBSOCKETCAN_BUILD_DIR   "${LIBSOCKETCAN_PREFIX_DIR}/build")
set(LIBSOCKETCAN_INSTALL_DIR "${LIBSOCKETCAN_PREFIX_DIR}/install")

set(LIBSOCKETCAN_INCLUDE_DIR "${LIBSOCKETCAN_INSTALL_DIR}/include")
set(LIBSOCKETCAN_LIBRARY     "${LIBSOCKETCAN_INSTALL_DIR}/lib/libsocketcan.a")

# Avoid CMake errors for imported targets whose include directory does not exist
# yet at configure time.
file(MAKE_DIRECTORY "${LIBSOCKETCAN_INCLUDE_DIR}")
file(MAKE_DIRECTORY "${LIBSOCKETCAN_INSTALL_DIR}/lib")

ExternalProject_Add(libsocketcan_external
    URL "${LIBSOCKETCAN_URL}"
    ${_libsocketcan_url_hash}

    DOWNLOAD_EXTRACT_TIMESTAMP FALSE

    SOURCE_DIR "${LIBSOCKETCAN_SOURCE_DIR}"
    BINARY_DIR "${LIBSOCKETCAN_BUILD_DIR}"
    INSTALL_DIR "${LIBSOCKETCAN_INSTALL_DIR}"

    CONFIGURE_COMMAND
        ${CMAKE_COMMAND} -E chdir <SOURCE_DIR>
            ${SH_EXE} ./autogen.sh
        COMMAND
        ${CMAKE_COMMAND} -E env
            "CFLAGS=-fPIC"
            "CXXFLAGS=-fPIC"
            <SOURCE_DIR>/configure
                --prefix=<INSTALL_DIR>
                --enable-static
                --disable-shared

    BUILD_COMMAND
        ${MAKE_EXE}

    INSTALL_COMMAND
        ${MAKE_EXE} install

    BUILD_BYPRODUCTS
        "${LIBSOCKETCAN_LIBRARY}"

    USES_TERMINAL_CONFIGURE TRUE
    USES_TERMINAL_BUILD TRUE
    USES_TERMINAL_INSTALL TRUE
)

add_library(libsocketcan STATIC IMPORTED GLOBAL)

set_target_properties(libsocketcan PROPERTIES
    IMPORTED_LOCATION "${LIBSOCKETCAN_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${LIBSOCKETCAN_INCLUDE_DIR}"
)

add_dependencies(libsocketcan libsocketcan_external)
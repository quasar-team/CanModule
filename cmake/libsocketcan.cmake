FetchContent_Declare(
  libsocketcan
  GIT_REPOSITORY https://github.com/linux-can/libsocketcan.git
  GIT_TAG        v0.0.12
  DOWNLOAD_EXTRACT_TIMESTAMP True
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

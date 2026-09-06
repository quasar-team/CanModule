include(FetchContent)

if(NOT DEFINED SYSTEC_LIBRARY)
    set(SYSTEC_LIBRARY "https://www.systec-electronic.com/media/default/Redakteur/produkte/Interfaces_Gateways/sysWORXX_USB_CANmodul_Series/Downloads/SO-387.zip")
endif()

set(SYSTEC_FETCHCONTENT_ARGS
    URL "${SYSTEC_LIBRARY}"
    DOWNLOAD_EXTRACT_TIMESTAMP True
)

if(EXISTS "${SYSTEC_LIBRARY}")
    message(STATUS "Using local Systec archive: ${SYSTEC_LIBRARY}")
elseif(SYSTEC_LIBRARY MATCHES "^https?://")
    message(STATUS "Downloading Systec archive: ${SYSTEC_LIBRARY}")
else()
    message(FATAL_ERROR "SYSTEC_LIBRARY must be an existing local archive or an http(s) URL. Got: ${SYSTEC_LIBRARY}")
endif()

FetchContent_Declare(
    Systec
    ${SYSTEC_FETCHCONTENT_ARGS}
)

FetchContent_MakeAvailable(Systec)

execute_process(
    COMMAND ${systec_SOURCE_DIR}/SO-387.exe /SP- /VERYSILENT /DIR=${systec_BINARY_DIR} /LOG=${systec_SOURCE_DIR}/build.log
    WORKING_DIRECTORY ${systec_SOURCE_DIR}
    RESULT_VARIABLE systec_build_result
    OUTPUT_VARIABLE systec_build_output
)

if(NOT systec_build_result EQUAL 0)
    message(FATAL_ERROR "Error installing USB-CANmodul Utility Disk: ${systec_build_output}")
endif()

if (WIN32)
    add_compile_definitions(WIN32)
endif()

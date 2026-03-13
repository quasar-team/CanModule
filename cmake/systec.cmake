include(FetchContent)

if(NOT DEFINED SYSTEC_LIBRARY)
	message(FATAL_ERROR "Could not find systec library locally")
    # set(SYTEC_LIBRARY "TODO zip path goes here")
endif()

set(SYSTEC_FETCHCONTENT_ARGS
    URL "${SYSTEC_LIBRARY}"
    DOWNLOAD_EXTRACT_TIMESTAMP True
)

if(EXISTS "${SYSTEC_LIBRARY}")
    message(STATUS "Using local Systec archive: ${SYSTEC_LIBRARY}")
elseif(SYSTEC_LIBRARY MATCHES "^https?://")
    if(DEFINED ENV{ICS_REPO_DEPS_TOKEN} AND NOT "$ENV{ICS_REPO_DEPS_TOKEN}" STREQUAL "")
        message(STATUS "Downloading Systec archive with authentication: ${SYSTEC_LIBRARY}")
        list(APPEND SYSTEC_FETCHCONTENT_ARGS
            HTTP_HEADER "PRIVATE-TOKEN: $ENV{ICS_REPO_DEPS_TOKEN}"
        )
    else()
        message(STATUS "Downloading Systec archive without authentication: ${SYSTEC_LIBRARY}")
    endif()
else()
    message(FATAL_ERROR "SYSTEC_LIBRARY must be an existing local archive or an http(s) URL. Got: ${SYSTEC_LIBRARY}")
endif()

FetchContent_Declare(
    Systec
    ${SYSTEC_FETCHCONTENT_ARGS}
)

FetchContent_MakeAvailable(Systec)

if (WIN32)
    add_compile_definitions(WIN32)
endif()

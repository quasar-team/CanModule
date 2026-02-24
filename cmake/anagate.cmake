include(FetchContent)

if(NOT DEFINED ANAGATE_LIBRARY)
    set(ANAGATE_LIBRARY "https://ics-deps-repo.web.cern.ch/canmodule/anagate/20241023/AnaGateAPI-CAN-2.5-BETA-3.zip")
endif()

set(ANAGATE_FETCHCONTENT_ARGS
    URL "${ANAGATE_LIBRARY}"
    DOWNLOAD_EXTRACT_TIMESTAMP True
)

if(EXISTS "${ANAGATE_LIBRARY}")
    message(STATUS "Using local Anagate archive: ${ANAGATE_LIBRARY}")
elseif(ANAGATE_LIBRARY MATCHES "^https?://")
    if(DEFINED ENV{ICS_REPO_DEPS_TOKEN} AND NOT "$ENV{ICS_REPO_DEPS_TOKEN}" STREQUAL "")
        message(STATUS "Downloading Anagate archive with authentication: ${ANAGATE_LIBRARY}")
        list(APPEND ANAGATE_FETCHCONTENT_ARGS
            HTTP_HEADER "PRIVATE-TOKEN: $ENV{ICS_REPO_DEPS_TOKEN}"
        )
    else()
        message(STATUS "Downloading Anagate archive without authentication: ${ANAGATE_LIBRARY}")
    endif()
else()
    message(FATAL_ERROR "ANAGATE_LIBRARY must be an existing local archive or an http(s) URL. Got: ${ANAGATE_LIBRARY}")
endif()

FetchContent_Declare(
    Anagate
    ${ANAGATE_FETCHCONTENT_ARGS}
)

FetchContent_MakeAvailable(Anagate)

if (WIN32)
    add_compile_definitions(WIN32)
    add_compile_definitions(ANAGATEDLL_EXPORTS)
endif()

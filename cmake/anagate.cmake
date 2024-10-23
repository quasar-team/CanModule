include(FetchContent)

set(ANAGATE_LIBRARY "https://ics-deps-repo.web.cern.ch/canmodule/anagate/20241023/AnaGateAPI-CAN-2.5-BETA-3.zip")

if(NOT DEFINED ENV{ICS_REPO_DEPS_TOKEN})
    message(FATAL_ERROR "Environment variable ICS_REPO_DEPS_TOKEN is not set. Please set this variable to proceed.")
endif()

message(STATUS "Download URL: ${ANAGATE_LIBRARY}")

FetchContent_Declare(
    Anagate
    URL ${ANAGATE_LIBRARY}
    HTTP_HEADER "PRIVATE-TOKEN: $ENV{ICS_REPO_DEPS_TOKEN}"
    DOWNLOAD_EXTRACT_TIMESTAMP True
)

FetchContent_MakeAvailable(Anagate)

if (WIN32)
    add_compile_definitions(WIN32)
endif()

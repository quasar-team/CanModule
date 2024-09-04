include(FetchContent)

set(ANAGATE_REPO_ROOT "https://ics-deps-repo.web.cern.ch/canmodule/anagate")
set(ANAGATE_WINDOWS_FILE "20240815-beta.v6-windows.zip")
set(ANAGATE_LINUX_FILE "20240815-beta.v6-linux.zip")

if(NOT DEFINED ENV{ICS_REPO_DEPS_TOKEN})
    message(FATAL_ERROR "Environment variable ICS_REPO_DEPS_TOKEN is not set. Please set this variable to proceed.")
endif()

if(WIN32)
    set(ANAGATE_URL "${ANAGATE_REPO_ROOT}/${ANAGATE_WINDOWS_FILE}")
elseif(UNIX)
    set(ANAGATE_URL "${ANAGATE_REPO_ROOT}/${ANAGATE_LINUX_FILE}")
else()
    message(FATAL_ERROR "Unsupported operating system. This script only supports Windows and Linux.")
endif()

message(STATUS "Download URL: ${ANAGATE_URL}")

FetchContent_Declare(
    Anagate
    URL ${ANAGATE_URL}
    HTTP_HEADER "PRIVATE-TOKEN: $ENV{ICS_REPO_DEPS_TOKEN}"
    DOWNLOAD_EXTRACT_TIMESTAMP True
)

FetchContent_MakeAvailable(Anagate)

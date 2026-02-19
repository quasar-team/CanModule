#-----
#SYSTEC USB-CANmodul Utility Disk
#-----
if( NOT DEFINED ENV{SYSTEC_PATH_HEADERS} OR NOT DEFINED ENV{SYSTEC_PATH_LIBS} )
	message( FATAL_ERROR "unable to determine SYSTEC USB-CANmodule Utility Disk headers and library paths from environment variables SYSTEC_PATH_HEADERS [$ENV{SYSTEC_PATH_HEADERS}] SYSTEC_PATH_LIBS [$ENV{SYSTEC_PATH_LIBS}]")
else()
	message( STATUS "using SYSTEC USB-CANmodule Utility Disk headers and library paths from environment variables SYSTEC_PATH_HEADERS [$ENV{SYSTEC_PATH_HEADERS}] SYSTEC_PATH_LIBS [$ENV{SYSTEC_PATH_LIBS}]")
endif()
include_directories($ENV{SYSTEC_PATH_HEADERS})
set(SYSTEC_LIB_PATH $ENV{SYSTEC_PATH_LIBS})
if(NOT TARGET libusbcan64)
	add_library(libusbcan64 SHARED IMPORTED)
	set_property(TARGET libusbcan64 PROPERTY IMPORTED_LOCATION $ENV{SYSTEC_PATH_LIBS}/USBCAN64.lib)
endif()
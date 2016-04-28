IF (WIN32)	
	add_library (CanInterface OBJECT
	        src/CanLibExternCWrapper.cpp
	        src/CanStatistics.cpp    
	        src/CCCUtils.cpp
	    	src/gettimeofday.cpp
			${CAN_LIBRARY_PLATFORM_IMPL}
		)        
ELSE()
	add_library (CanInterface OBJECT
	        src/CanLibExternCWrapper.cpp
	        src/CanStatistics.cpp    
	        src/CCCUtils.cpp    
			${CAN_LIBRARY_PLATFORM_IMPL}
		)
ENDIF()


# CMakeLists to generate python bindings for CanModule

set(PYTHON_SOURCES
    src/python/CanModule.cpp
)

# Check if pybind11 is installed using pip
execute_process(
    COMMAND ${CMAKE_COMMAND} -E env python -m pip show pybind11
    RESULT_VARIABLE pybind11_installed
    OUTPUT_VARIABLE pybind11_output
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)

if(pybind11_installed EQUAL 0)
  message(STATUS "pybind11 found, genering python module")

  # Extract the location of pybind11
  string(REGEX MATCH "Location: ([^\n]+)" _ ${pybind11_output})
  string(REGEX REPLACE "Location: " "" pybind11_location ${CMAKE_MATCH_1})

  set(pybind11_DIR ${pybind11_location}/pybind11/share/cmake/pybind11)

  find_package(pybind11 REQUIRED)
  find_package(Python3 REQUIRED COMPONENTS Development)

  pybind11_add_module(canmodule ${PYTHON_SOURCES})
  target_include_directories(canmodule PRIVATE ${logit_SOURCE_DIR}/include)
  target_link_libraries(canmodule PRIVATE CanModuleMain pybind11::module LogIt ${PYTHON_LIBRARIES})
endif()

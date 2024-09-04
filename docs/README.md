# Welcome

Welcome to the CANModule project, a C++ library that provides a simple and flexible interface for interacting with Controller Area Network (CAN) devices. This project is designed to be easy to use and can be easily integrated into the Quasar Framework.

The project's overall structure is organized into several key directories and subdirectories, including ci, cmake, docs, and src.

- `ci`: Contains scripts and configuration files related to the project's continuous integration (CI) process.
- `cmake`: Contains CMake modules that are used for configuring and building the project.
- `docs`: Contains documentation files for the project, including the primary Doxyfile used for generating API documentation.
- `src`: The main source code directory, containing header files, main source code files, and Python-related bindings.
- `test`: Contains test files for the project, including subdirectories for C++ and Python tests.

The project supports currently Anagate (Windows and Linux) and Socketcan (only Linux). It provides a clean and intuitive interface for sending and receiving CAN frames, as well as for configuring and monitoring the CAN bus.

To get started with the project, please refer to the project's documentation, including the Quickstart, the Developer Guide, the API documentation, and the examples provided in the test directory.

## Links

- Documentation page: https://canmodule.docs.cern.ch
- Github page: https://github.com/quasar-team/canmodule
- CERN Gitlab mirror: https://gitlab.cern.ch/quasar-team/canmodule

## Requirements

The project uses the standard library, C++17 and has been tested using Visual Studio 2022 and
GCC 11.4.

The project requires LogIt (part of the Quasar Framework) and it is downloaded automatically by CMake.

Inside CERN, the CMake will also download the Anagate API. If not, please modify the address to
download the API directly from anagate.de

For Linux, the user must install `libsocketcan` and the matching devel package.

Optionally, the user must install pybind11 to generate the Python bindings.

## Tests

The core of the project is tested with the Google Test Framework, while the vendor implementation is
tested using Python bindings (Virtual Socketcan and Anagate). Please refer to the `test` directory for
more information.

## Python bindings

CanModule provides access to all public members and methods via Python. The naming convention is
the same as the main C++ code. Please follow the main C++ documentation while in Python, and be
aware of minor differences due to the language, such as `std::vector` is a `list`, `std::map` is a `dict`, etc. in Python.

## Contact

E-Mail: <icecontrols.support@cern.ch>

Unit: BE-ICS-TMA, CERN

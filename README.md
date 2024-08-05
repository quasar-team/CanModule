
# Developer Guide

## Development environment

- Alma 9
- Visual Studio Code
- Extension VS: TestMate C++

Windows works too for development, installing all dependencies with Chocolatey, but you cannot
compile (nor debug) any issues with SocketCan.

## Setup pre-commit hooks

Tasks such as formatting of the code, removing unnecesary whitespaces and doing some sanity checks are performed by pre-commit.com

```
pip install pre-commit clang-format
pre-commit install-hooks
pre-commit install
```

You will also need `cppcheck` in your system (`dnf install cppcheck`)

## Python Bindings

Install `pybind11` if you wish to have the python bindings compiled

- `pip install pybind11`
- `dnf install python3-devel`
- `pip install pytest`

## Documentation

Install `doxygen`, enter in the directory `docs/` and run `doxygen`.

The output is located at `build/docs`.

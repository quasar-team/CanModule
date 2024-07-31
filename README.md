
## Development environment

- Alma 9
- Visual Studio Code
- Extension VS: TestMate C++ (optional)

## Setup pre-commit hooks

Tasks such as formatting of the code, removing unnecesary whitespaces and doing some sanity checks are performed by pre-commit.com

```
pip install pre-commit clang-format
pre-commit install-hooks
pre-commit install
```

Install `cppcheck` in your system

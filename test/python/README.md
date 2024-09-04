# Information regarding the stubs

The stubs present in the `test/python/canmodule` folder are OPTIONAL.

The objective of the stubs are only to make pylint (and by extension Visual Studio Code) happy,
avoiding warning of not existing methods or variables and provide autocompletion.

The absence and/or false information of the stubs are completelly irrelevant once you run
the tests (or any program using the python bindings). The leading information is the python binary
library of `canmodule` in the top-level `build` folder.

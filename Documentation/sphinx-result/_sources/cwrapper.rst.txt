=========
C-Wrapper
=========

in order to call **CanModule** also from low-level vendor specific code, which is often implemented
in C, CanModule has a C-wrapper as well. The objective is to call the C++ objects of CanModule
from C-code. Still, a C++ compiler is needed for the build, but this type of
compiler is usually downward compatible to C.


#!/bin/csh
# using the doxygen xml in sphinx-source as input for sphinx
set SPHINX_BUILD=/usr/local/bin/sphinx-build
${SPHINX_BUILD} -b html ./sphinx-source ./sphinx-result/
mkdir -p ./sphinx-result/latex
${SPHINX_BUILD} -b latex ./sphinx-source ./sphinx-result/latex
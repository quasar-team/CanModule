#!/bin/csh
# make all the doc
# 1.doxygen xml
# 2.sphinx html
set SPHINX_BUILD=/usr/local/bin/sphinx-build
#cd .. && doxygen
#cd Documentation && 
${SPHINX_BUILD} -b html ./sphinx-source ./sphinx-result/
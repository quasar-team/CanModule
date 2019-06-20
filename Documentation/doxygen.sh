#!/bin/csh
# make all the doc
# 1.doxygen xml
cd .. && doxygen
echo "====================================================================="
echo "the doxygen tree is in Documentation/sphinx-source/doxyger-result/xml"
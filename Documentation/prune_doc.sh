#!/bin/csh
# prune documentation to keep just the html result
rm -rf ./sphinx-result/latex/
rm -rf ./sphinx-result/.doctrees/*
rm -rf ./sphinx-source/doxygen-result/

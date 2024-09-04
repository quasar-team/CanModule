import sys
import os

# Add the path to the directory containing the module
sys.path.append(os.path.join(os.getcwd(), "build"))
sys.path.append(os.path.join(os.getcwd(), "build", "Debug"))
sys.path.append(os.path.join(os.getcwd(), "build", "Release"))

from canmodule import *

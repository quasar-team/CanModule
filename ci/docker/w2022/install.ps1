$ErrorActionPreference = "Stop"

# Install chocolatey
Set-ExecutionPolicy Bypass -Scope Process -Force; [System.Net.ServicePointManager]::SecurityProtocol = [System.Net.ServicePointManager]::SecurityProtocol -bor 3072; iex ((New-Object System.Net.WebClient).DownloadString('https://community.chocolatey.org/install.ps1'))

# Miscelaneous dependencies
choco install -y --no-progress powershell-core python git git-lfs.install nano

# Install CMake and Ninja
choco install -y --no-progress cmake --installargs 'ADD_CMAKE_TO_PATH=System'
choco install -y --no-progress ninja

# Visual Studio 2019 Build Tools
choco install -y --no-progress visualstudio2022buildtools
choco install -y --no-progress visualstudio2022-workload-vctools

# Pip dependencies
Import-Module $env:ChocolateyInstall\helpers\chocolateyProfile.psm1
refreshenv

python -m pip install pybind11 pytest

# Clean up
choco cache remove

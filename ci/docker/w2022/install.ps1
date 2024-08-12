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

# C++ Component for Visual Studio 2022 with all extras
choco install -y --no-progress visualstudio2022-workload-vctools

# Link Professional folder to BuildTools for compatibility with existing
# building scripts
New-Item -ItemType SymbolicLink -Path "C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional" -Target "C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools"

# Clean-up
Remove-Item -Path "C:\Program Files (x86)\Microsoft Visual Studio\Installer" -Recurse -Force
Remove-Item -Path "C:\Users\ContainerAdministrator\AppData\Local\Temp\chocolatey" -Recurse -Force

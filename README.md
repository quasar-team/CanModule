
# Developer Guide

## Development environment

- Alma 9
- Visual Studio Code
- Extension VS: TestMate C++ (recommended)

Windows works too for development, installing all dependencies with Chocolatey, but you cannot
compile (nor debug) any issues with SocketCan.

## Setup pre-commit hooks (recommended)

Tasks such as formatting of the code, removing unnecesary whitespaces and doing some sanity checks are performed by pre-commit.com

```bash
pip install pre-commit clang-format
pre-commit install-hooks
pre-commit install
```

You will also need `cppcheck` in your system (`dnf install cppcheck`)

## Python Bindings (optional)

Install `pybind11` if you wish to have the python bindings compiled

- `pip install pybind11`
- `dnf install python3-devel`
- `pip install pytest`

## Documentation (optional)

Install `doxygen`, enter in the directory `docs/` and run `doxygen`.

The output is located at `build/docs`.

## Coverage (optional)

- `dnf install gcov`
- Run the target `coverage`
- Use the VS Code extension: Gcov Viewer from Jacques Lucke to highligh the covered code

## SocketCan setup (optional)

### Virtual SocketCan

In Linux it is convenient to use Virtual Can to test SocketCan implementation.

Install:

```bash
sudo dnf install iproute kernel-modules-extra
sudo dnf groupinstall "Development Tools"
sudo dnf install kernel-devel kernel-headers elfutils-libelf-devel
sudo dnf install --source kernel
```

Extract the `rpm --install` and go to `/root/rpmbuild/SOURCES/[your linux version]/drivers/net/can`

**NOTE:** Make sure your running kernel matches the source code extracted

Execute:

```bash
sudo make -C /lib/modules/$(uname -r)/build M=$(pwd) modules
sudo cp vcan.ko /lib/modules/$(uname -r)/kernel/drivers/net/can/
```

Configure:

```bash
sudo modprobe vcan
sudo ip link add dev vcan0 type vcan
sudo ip link set up vcan0
```

### PEAK

You can borrow a PEAK device from the lab with two CAN ports and connect between them (remember the termination).

Support for PEAK is included in Alma 9 `sudo modprobe peak_usb`

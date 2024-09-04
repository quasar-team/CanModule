
# Developer Guide

## Dependencies

- In Linux, you must install the `libsocketcan-devel` package.
- Other dependencies are resolved automatically after setting the variable `ICS_REPO_DEPS_TOKEN=[value]` to
your environment (`.env` file). The value is in the group `jcop-opc-ua` configuration of Gitlab CI/CD (gitlab.cern.ch).

## Development environment

- Alma 9
- Visual Studio Code
- Extension VS: TestMate C++ (recommended)

Windows works too for development, installing all dependencies with Chocolatey, but you cannot
compile (nor debug) with SocketCan support.

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

## SocketCan setup (optional)

### Virtual SocketCan

In Linux it is convenient to use Virtual Can to test SocketCan implementation.

Install:

```bash
sudo dnf install iproute kernel-modules-extra
sudo dnf groupinstall "Development Tools"
sudo dnf install kernel-devel kernel-headers elfutils-libelf-devel
sudo dnf download --source kernel
```

Extract the downloaded RPM with `rpm --install`. Go to `/root/rpmbuild/SOURCES/` extract the `.tar.xf` file and go to the `drivers/net/can` folder; otherwise you will need either to upgrade your
system or download the exact kernel RPM source code of your existing kernel.

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

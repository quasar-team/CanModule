# CanModule-Utils

The `canmodule-utils.py` script, located in the `python` folder of this repository, provides a command-line interface for various CAN (Controller Area Network) operations. These include sending messages, generating and sending random messages, printing all received messages, and displaying diagnostic information.

The tool is loosely based on `can-utils` for SocketCAN on Linux.

## Installation

To use this tool, ensure you have a recent version of Python installed (tested with version 3.9.18). Additionally, you need to have `libsocketcan` installed on your Linux system.

You will also require the `canmodule.cpython*` file and all Anagate-related libraries (such as `.so` files on Linux or `.dll` files on Windows). These files can be found in the build artifacts, located in the `/build` directory on Linux and the `/build/Release` directory on Windows.

## Usage

### dump

It opens a connection to the device and print all received frames. The syntax is:

```bash
python canmodule-utils.py anagate [host] [port number] dump
python canmodule-utils.py socketcan [device] dump
```

### send

It sends a single CAN frame. The syntax is:

```bash
python canmodule-utils.py anagate [host] [port number] send [can_frame]
python canmodule-utils.py socketcan [device] send [can_frame]
```

Examples of CAN frames are:

```bash
001#AA // Message of 1 byte (AA) and standard id 1
FFFA#BB.BB // Message of 2 bytes (BB BB) and extended if FFFA
001#R4 // Remote request message with length 4 and standard id 1
```

### gen

It opens a connection and send random frames. The syntax is:

```bash
python canmodule-utils.py anagate [host] [port number] gen
python canmodule-utils.py socketcan [device] gen
```

### diag

It opens a connection and print the diagnostics. The syntax is:

```bash
python python canmodule-utils.py.py anagate [host] [port number] diag
python python canmodule-utils.py.py socketcan [device] diag
```

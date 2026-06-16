# CanModule-Utils

The `canmodule-utils.py` script, located in the `python` folder of this repository, provides a command-line interface for various CAN (Controller Area Network) operations. These include sending messages, generating and sending random messages, printing all received messages, and displaying diagnostic information.

The tool is loosely based on `can-utils` for SocketCAN on Linux.

## Installation

To use this tool, ensure you have a recent version of Python installed (tested with version 3.9.18). Additionally, you need to have `libsocketcan` installed on your Linux system.

You will also require the `canmodule.cpython*` file and all Anagate-related libraries (such as `.so` files on Linux or `.dll` files on Windows). These files can be found in the build artifacts, located in the `/build` directory on Linux and the `/build/Release` directory on Windows.

## Usage

The syntax is:

```bash
python canmodule-utils.py <vendor> <action> [action arguments] [--config FILE] [--set KEY=VALUE ...]
```

Supported vendors are `anagate` and `socketcan`. Supported actions are `dump`, `send`, `gen`, and `diag`.

Device configuration is provided with `--set KEY=VALUE`, `--config FILE`, or both. `--set` can be repeated and the last value wins. When both mechanisms are used, JSON values are loaded first and `--set` values override them.

`--set` accepts the Python binding configuration field names:

```text
bus_name
bus_number
host
bitrate
enable_termination
high_speed
timeout
vcan
sent_acknowledgement
```

Hyphenated key spelling is also accepted, for example `--set enable-termination=true`.

JSON config files are flat objects that map directly to these fields:

```json
{
  "bus_name": "can0",
  "bitrate": 500000,
  "timeout": 100,
  "vcan": false
}
```

```json
{
  "host": "192.168.1.20",
  "bus_number": 0,
  "bitrate": 250000,
  "enable_termination": true,
  "high_speed": false,
  "timeout": 6000,
  "sent_acknowledgement": 0
}
```

Use JSON `null` or `--set key=null` to leave an optional configuration field unset.

### dump

It opens a connection to the device and print all received frames. The syntax is:

```bash
python canmodule-utils.py anagate dump --set host=192.168.1.20 --set bus_number=0
python canmodule-utils.py socketcan dump --set bus_name=can0
```

### send

It sends a single CAN frame. The syntax is:

```bash
python canmodule-utils.py anagate send [can_frame] --config anagate0.json
python canmodule-utils.py socketcan send [can_frame] --set bus_name=can0
```

Examples of CAN frames are:

```bash
001#AA // Message of 1 byte (AA) and standard id 1
FFFA#BB.BB // Message of 2 bytes (BB BB) and extended id FFFA
001#R4 // Remote request message with length 4 and standard id 1
00000123#AA // Message of 1 byte (AA) with small ID sent as extended format
00000123#R4 // Remote request of length 4 with small ID sent as extended format
```

### gen

It opens a connection and send random frames. The syntax is:

```bash
python canmodule-utils.py anagate gen --set host=192.168.1.20 --set bus_number=0
python canmodule-utils.py socketcan gen --set bus_name=can0
```

### diag

It opens a connection and print the diagnostics. The syntax is:

```bash
python canmodule-utils.py anagate diag --set host=192.168.1.20 --set bus_number=0
python canmodule-utils.py socketcan diag --set bus_name=can0
```

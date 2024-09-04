# Quick Start

## Quick Start in C++

This quick start guide should help you get up and running with the `CanFrame` and `CanDevice` classes. For more advanced usage and configuration options, refer to the full documentation and the unit tests.

### Creating a CanFrame object

You can create a `CanFrame` object using several constructors, depending on the input parameters:

#### Constructor with ID and Message

```cpp
CanFrame frame(1, {'H', 'e', 'l', 'l', 'o'});
```

- `frame`: A `CanFrame` object initialized with the given standard id and message.

#### Constructor with Extended ID and Message

```cpp
CanFrame frame(1<<28, {'H', 'e', 'l', 'l', 'o'});
```

- This creates a `CanFrame` object with an extended identifier and message.

#### Constructor with ID and Remote Request

```cpp
CanFrame frame(1, 8);
```

- This creates a remote request (length 8) of a frame with id 1.

#### Constructor with ID, Message, and Flags

```cpp
CanFrame frame(1, {'H', 'e', 'l', 'l', 'o'}, can_flags::extended_id);
```

- `flags`: Special flags (e.g., `standard_id`, `extended_id`, `remote_request`) that define the frame type.
- This constructor allows for more advanced frame configurations.

### Using the CanFrame Methods

After creating a `CanFrame` object, you can use various methods to retrieve its properties and check its type:

#### Accessing the Frame ID

```cpp
frame.id();
```

- Returns the identifier of the CAN frame.

#### Accessing the Message

```cpp
frame.message();
```

- Returns the message vector contained in the frame.

#### Checking the Frame Type

```cpp
frame.is_standard_id(); // Returns true if the frame uses a standard ID.
frame.is_extended_id(); // Returns true if the frame uses an extended ID.
frame.is_remote_request(); // Returns true if the frame is a remote request (RTR).
```

#### Checking Flags

```cpp
frame.flags();
```

- Returns the flags associated with the frame, indicating its type and properties.

#### Comparing CanFrame Objects

```cpp
CanFrame frame1(id);
CanFrame frame2(id);
assert(frame1 == frame2); // Check if two frames are equal.
assert(frame1 != frame2); // Check if two frames are not equal.
```

### Error Handling

The `CanFrame` class will throw exceptions for invalid operations, such as:

#### Invalid ID or Message Size

If the `id` is too large or the message size exceeds the allowed limit, the constructor will throw a `std::invalid_argument`.

#### Invalid Flag Combinations

If the provided flags do not align with the frame type (e.g., using standard id, and providing an ID that exceeds 11 bits), an exception will be thrown.

### Creating a CAN Device

To create a CAN device, you can use the static `create` method provided by the `CanDevice` class. The method requires the name of the device type and a `CanDeviceArguments` structure, which includes the configuration and a callback for handling incoming frames.

```cpp
#include "CanDevice.h"

auto dummy_cb_ = [](const CanFrame& frame) { /* Handle incoming frame */ };
auto myDevice = CanDevice::create(
    "loopback",
    CanDeviceArguments{CanDeviceConfiguration{"dummy"}, dummy_cb_});
```

In this example, a loopback device is created. The `dummy_cb_` callback function is defined to handle any incoming frames.

### Configuring the CAN Device

The configuration of a CAN device is done through the `CanDeviceConfiguration` structure, which is passed as part of the `CanDeviceArguments`. In this example, the configuration specifies the `bus_name`.

```cpp
CanDeviceConfiguration config{"dummy"};
CanDeviceArguments args{config, dummy_cb_};
auto myDevice = CanDevice::create("loopback", args);
```

In this example, the `bus_name` is set to `"dummy"`. You can customize the configuration based on the specific requirements of your CAN bus setup.

### Sending CAN Frames

To send CAN frames, you can use the `send` method of the `CanDevice` class. This method takes a vector of `CanFrame` objects.

```cpp
std::vector<CanFrame> outFrames;

for (uint32_t i = 0; i < 10; ++i) {
    outFrames.push_back(CanFrame{i});
}

myDevice->send(outFrames);
```

In this example, ten `CanFrame` objects are created and sent through the CAN device.

### Receiving CAN Frames

Incoming CAN frames can be received and processed using the callback function provided during device creation. The frames are typically handled inside this callback, where they can be stored, logged, or otherwise processed.

```cpp
std::vector<CanFrame> inFrames;

auto dummy_cb_ = [&inFrames](const CanFrame& frame) {
    inFrames.push_back(frame);
};
auto myDevice = CanDevice::create(
    "loopback",
    CanDeviceArguments{CanDeviceConfiguration{"dummy"}, dummy_cb_});

```

In this example, incoming frames are stored in the `inFrames` vector for later processing.

### Complete Example

Below is a complete example demonstrating the creation of a CAN device, configuring it, and sending/receiving CAN frames.

```cpp
#include "CanDevice.h"

std::vector<CanFrame> outFrames;
std::vector<CanFrame> inFrames;

auto dummy_cb_ = [&inFrames](const CanFrame& frame) {
    inFrames.push_back(frame);
};
auto myDevice = CanDevice::create(
    "loopback",
    CanDeviceArguments{CanDeviceConfiguration{"dummy"}, dummy_cb_});

for (uint32_t i = 0; i < 10; ++i) {
    outFrames.push_back(CanFrame{i});
}

myDevice->send(outFrames);

// Verify that the frames sent match the frames received
for (int i = 0; i < 10; ++i) {
    assert(outFrames[i] == inFrames[i]);
}
```

In this example:

- A loopback CAN device is created.
- Frames are sent and received using the `send` method.
- The example verifies that the sent frames match the received frames.

## Quick Start in Python

CanModule provides bindings of the majority of C++ classes, methods and members for Python.

### Creating CAN Device Configurations

First, you need to create configurations for your CAN devices. In this example, two devices are configured to use the virtual CAN interface `vcan0`.

```python
from canmodule import CanDeviceConfiguration

DEVICE_ONE = CanDeviceConfiguration()
DEVICE_ONE.bus_name = "vcan0"

DEVICE_TWO = CanDeviceConfiguration()
DEVICE_TWO.bus_name = "vcan0"
```

### Creating and Opening CAN Devices

Once the device configurations are set up, you can create and open CAN devices using the `CanDevice.create` method. This method requires the protocol (`socketcan` in this case), the device configuration, and a callback function to handle received frames.

```python
from canmodule import CanDevice, CanDeviceArguments

received_frames_dev1 = []
received_frames_dev2 = []

myDevice1 = CanDevice.create(
    "socketcan", CanDeviceArguments(DEVICE_ONE, received_frames_dev1.append)
)
myDevice2 = CanDevice.create(
    "socketcan", CanDeviceArguments(DEVICE_TWO, received_frames_dev2.append)
)

myDevice1.open()
myDevice2.open()
```

## Sending and Receiving CAN Messages

### Sending a Single CAN Frame

You can send a single CAN frame using the `send` method of the `CanDevice` class. The frame is defined using the `CanFrame` class, which includes the message ID and the data payload.

```python
from canmodule import CanFrame

myDevice1.send(CanFrame(123, ["H", "e", "l", "l", "o"]))
```

### Verifying Reception of CAN Frames

To ensure that the CAN frame was received correctly, you can check the contents of the `received_frames_dev2` list, which was populated by the callback function.

```python
assert len(received_frames_dev2) == 1
assert received_frames_dev2[0].id() == 123
assert received_frames_dev2[0].message() == ["H", "e", "l", "l", "o"]
```

### Sending Multiple CAN Frames

You can also send multiple CAN frames at once by passing a list of `CanFrame` objects to the `send` method.

```python
send_frames = [
CanFrame(123, ["H", "e", "l", "l", "o"]),
CanFrame(234, ["W", "o", "r", "l", "d"]),
CanFrame(345, 5),
CanFrame(456, ["J", "u", "s", "t"], can_flags.extended_id),
CanFrame(1 << 25, ["J", "u", "s", "t"], can_flags.extended_id),
]

myDevice1.send(send_frames)
```

### Checking the Received Frames

You can then verify that the frames were received correctly by checking the `received_frames_dev2` list.

```python
assert len(received_frames_dev2) == 5

# Check first frame
assert received_frames_dev2[0].id() == 123
assert received_frames_dev2[0].message() == ["H", "e", "l", "l", "o"]

# Check second frame
assert received_frames_dev2[1].id() == 234
assert received_frames_dev2[1].message() == ["W", "o", "r", "l", "d"]

# And so on for other frames...
```

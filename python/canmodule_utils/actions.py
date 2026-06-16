import time

from canmodule import CanDevice, CanDeviceArguments, CanReturnCode

from .frames import generate_random_frame, parse_can_frame, process_frame


def create_can_device(vendor, configuration, receiver=None):
    arguments = CanDeviceArguments(configuration, receiver)
    return CanDevice.create(vendor, arguments)


def ensure_open(can_device):
    if can_device.open() != CanReturnCode.success:
        print("Error opening CAN device")
        raise SystemExit(1)


def dump(vendor, configuration):
    can_device = create_can_device(vendor, configuration, print)
    ensure_open(can_device)

    try:
        print("**** Dump started. Press CTRL+C to stop. ****")
        while True:
            time.sleep(5)
    except KeyboardInterrupt:
        print("\nCTRL+C pressed. Exiting gracefully...")
        can_device.close()


def send(vendor, configuration, frame):
    can_frame = process_frame(frame)
    can_device = create_can_device(vendor, configuration)
    ensure_open(can_device)

    if can_device.send(can_frame) != CanReturnCode.success:
        print("Error sending frame")
        raise SystemExit(1)

    print(f"Sent frame: {can_frame}")


def gen(vendor, configuration):
    can_device = create_can_device(vendor, configuration)
    ensure_open(can_device)

    try:
        print("**** Generate and send random frames. Press CTRL+C to stop. ****")
        while True:
            time.sleep(1)
            random_frame = generate_random_frame()
            can_frame = process_frame(parse_can_frame(random_frame))
            print(f"Generated and sent frame: {can_frame}")
            can_device.send(can_frame)
    except KeyboardInterrupt:
        print("\nCTRL+C pressed. Exiting gracefully...")
        can_device.close()


def diag(vendor, configuration):
    can_device = create_can_device(vendor, configuration)
    ensure_open(can_device)

    print(can_device.diagnostics())
    can_device.close()

import argparse
import time
import random

from canmodule import *


def parse_arguments():
    """
    Parses command-line arguments for the CAN Module Utilities.

    The function uses argparse to define and parse command-line arguments for different CAN module operations.
    It supports two CAN modules: Anagate and SocketCAN. Each module has its own subcommands for different actions,
    such as dumping frames, generating random frames, sending specified frames, and printing diagnostics.

    Parameters:
    None

    Returns:
    argparse.Namespace: An object containing the parsed command-line arguments.
    """
    parser = argparse.ArgumentParser(description="CAN Module Utilities")

    # Main subparsers
    subparsers = parser.add_subparsers(
        dest="command", required=True, help="sub-command help"
    )

    # anagate subparser
    anagate_parser = subparsers.add_parser("anagate", help="Use the Anagate CAN module")
    anagate_parser.add_argument("host", type=str, help="Host address for Anagate")
    anagate_parser.add_argument("port", type=int, help="Port number for Anagate")

    anagate_subparsers = anagate_parser.add_subparsers(
        dest="action", required=True, help="Anagate actions"
    )

    anagate_subparsers.add_parser("dump", help="Dump frames")
    anagate_subparsers.add_parser("diag", help="Print diagnostics")
    anagate_subparsers.add_parser("gen", help="Generate and send random frames")

    anagate_send_parser = anagate_subparsers.add_parser(
        "send", help="Send a specified frame"
    )
    anagate_send_parser.add_argument("can_frame", type=str, help="CAN frame to send")

    # socketcan subparser
    socketcan_parser = subparsers.add_parser(
        "socketcan", help="Use the SocketCAN module"
    )
    socketcan_parser.add_argument("device", type=str, help="Device name for SocketCAN")

    socketcan_subparsers = socketcan_parser.add_subparsers(
        dest="action", required=True, help="SocketCAN actions"
    )

    socketcan_subparsers.add_parser("dump", help="Dump frames")
    socketcan_subparsers.add_parser("diag", help="Print diagnostics")
    socketcan_subparsers.add_parser("gen", help="Generate and send random frames")

    socketcan_send_parser = socketcan_subparsers.add_parser(
        "send", help="Send a specified frame"
    )
    socketcan_send_parser.add_argument("can_frame", type=str, help="CAN frame to send")

    return parser.parse_args()


def main():
    """
    The main function of the CAN Module Utilities. It parses command-line arguments,
    initializes the CAN device based on the provided arguments, and performs the specified action.

    Parameters:
    None

    Returns:
    None
    """
    args = parse_arguments()

    device = {
        "vendor": args.command,
    }

    if args.command == "anagate":
        device.update(
            {
                "host": args.host,
                "port": args.port,
            }
        )
    elif args.command == "socketcan":
        device.update(
            {
                "device": args.device,
            }
        )

    if args.action == "dump":
        dump(device)
    elif args.action == "send":
        frame = parse_can_frame(args.can_frame)
        send(device, frame)
    elif args.action == "gen":
        gen(device)
    elif args.action == "diag":
        diag(device)


def parse_can_frame(can_frame_str):
    """
    Parses a CAN frame string into a dictionary representation.

    The function takes a string representing a CAN frame in the format "CAN_ID#DATA" or "CAN_ID#Rlen",
    where CAN_ID is the hexadecimal representation of the CAN identifier, DATA is the hexadecimal representation
    of the CAN data bytes separated by periods, and len is the number of data bytes for a remote request frame.

    Parameters:
    can_frame_str (str): The CAN frame string to be parsed.

    Returns:
    dict: A dictionary representing the parsed CAN frame. The dictionary contains the following keys:
          - "can_id" (str): The hexadecimal representation of the CAN identifier.
          - "len" (int): The number of data bytes for a remote request frame.
          - "data" (list of str): The hexadecimal representation of the CAN data bytes for a data frame.
    """
    if "R" in can_frame_str:
        can_id, len_data = can_frame_str.split("#R")
        frame = {
            "can_id": can_id,
            "len": int(len_data),
        }
    else:
        can_id, data = can_frame_str.split("#")
        frame = {
            "can_id": can_id,
            "data": data.split("."),
        }
    return frame


def process_device(device):
    """
    Processes the device dictionary to create a CanDeviceConfiguration object.

    This function takes a dictionary representing a CAN device and creates a CanDeviceConfiguration object
    based on the provided device information. The function supports two types of CAN devices: Anagate and SocketCAN.
    For Anagate devices, the function sets the host and bus number in the configuration object. For SocketCAN devices,
    the function sets the bus name in the configuration object. If an unsupported vendor is provided, the function
    prints an error message and exits with a status code of 1.

    Parameters:
    device (dict): A dictionary representing the CAN device. The dictionary should contain the following keys:
                   - "vendor" (str): The vendor of the CAN device. It can be either "anagate" or "socketcan".
                   - "host" (str): The host address for Anagate devices.
                   - "port" (int): The port number for Anagate devices.
                   - "device" (str): The device name for SocketCAN devices.

    Returns:
    CanDeviceConfiguration: A CanDeviceConfiguration object configured based on the provided device information.
    """
    configuration = CanDeviceConfiguration()
    if device["vendor"] == "anagate":
        configuration.host = device["host"]
        configuration.bus_number = device["port"]
    elif device["vendor"] == "socketcan":
        configuration.bus_name = device["device"]
    else:
        print(f"Unsupported vendor: {device['vendor']}")
        exit(1)
    return configuration


def dump(device):
    """
    This function opens a CAN device, starts printing received CAN frames, and handles user interruption.

    Parameters:
    device (dict): A dictionary representing the CAN device. The dictionary should contain the following keys:
                   - "vendor" (str): The vendor of the CAN device. It can be either "anagate" or "socketcan".
                   - "host" (str): The host address for Anagate devices.
                   - "port" (int): The port number for Anagate devices.
                   - "device" (str): The device name for SocketCAN devices.

    Returns:
    None
    """
    configuration = process_device(device)
    arguments = CanDeviceArguments(configuration, print)
    can_device = CanDevice.create(device["vendor"], arguments)
    
    if (can_device.open() != CanReturnCode.success):
        print("Error opening CAN device")
        exit(1)

    try:
        print("**** Dump started. Press CTRL+C to stop. ****")
        while True:
            time.sleep(5)
    except KeyboardInterrupt:
        print("\nCTRL+C pressed. Exiting gracefully...")
        can_device.close()


def process_frame(frame):
    """
    Processes a CAN frame dictionary into a CanFrame object.

    This function takes a dictionary representing a CAN frame and creates a CanFrame object
    based on the provided frame information. The function determines the CAN identifier type (standard or extended),
    sets the appropriate flags, and constructs the CanFrame object.

    Parameters:
    frame (dict): A dictionary representing the CAN frame. The dictionary should contain the following keys:
                  - "can_id" (str): The hexadecimal representation of the CAN identifier.
                  - "len" (int): The number of data bytes for a remote request frame.
                  - "data" (list of str): The hexadecimal representation of the CAN data bytes for a data frame.

    Returns:
    CanFrame: A CanFrame object constructed based on the provided frame information.
    """
    can_id = int(frame["can_id"], 16)

    extended_id = False
    if can_id > 0x7FF:  # More than 11 bits long
        extended_id = True

    remote_request = False
    flags = 0

    if "len" in frame.keys():
        remote_request = True
        flags |= can_flags.remote_request if remote_request else 0
        flags |= can_flags.extended_id if extended_id else can_flags.standard_id
        can_frame = CanFrame(can_id, frame["len"], flags)
    else:
        data = [chr(int(byte, 16)) for byte in frame["data"]]
        flags |= can_flags.extended_id if extended_id else can_flags.standard_id
        can_frame = CanFrame(can_id, data, flags)

    return can_frame


def send(device, frame):
    """
    Sends a CAN frame to the specified device.

    This function takes a device dictionary and a CAN frame dictionary as input, processes them,
    and sends the CAN frame to the specified device using the provided CAN module.

    Parameters:
    device (dict): A dictionary representing the CAN device. The dictionary should contain the following keys:
                   - "vendor" (str): The vendor of the CAN device. It can be either "anagate" or "socketcan".
                   - "host" (str): The host address for Anagate devices.
                   - "port" (int): The port number for Anagate devices.
                   - "device" (str): The device name for SocketCAN devices.
    frame (dict): A dictionary representing the CAN frame to be sent. The dictionary should contain the following keys:
                  - "can_id" (str): The hexadecimal representation of the CAN identifier.
                  - "len" (int): The number of data bytes for a remote request frame.
                  - "data" (list of str): The hexadecimal representation of the CAN data bytes for a data frame.

    Returns:
    None
    """
    can_frame = process_frame(frame)
    configuration = process_device(device)
    arguments = CanDeviceArguments(configuration, None)
    can_device = CanDevice.create(device["vendor"], arguments)

    if (can_device.open() != CanReturnCode.success):
        print("Error opening CAN device")
        exit(1)
    
    if (can_device.send(can_frame) != CanReturnCode.success):
        print("Error sending frame")
        exit(1)

    print(f"Sent frame: {can_frame}")


def generate_random_frame():
    """
    Generates a random CAN frame in the format "CAN_ID#DATA" or "CAN_ID#Rlen".

    The function randomly selects the CAN identifier type (standard or extended), generates a random CAN identifier,
    decides whether to send a remote request frame, and generates random data bytes for the frame. The generated frame
    is then returned as a string.

    Parameters:
    None

    Returns:
    str: A string representing a random CAN frame in the format "CAN_ID#DATA" or "CAN_ID#Rlen".
    """
    id_type = random.choice(["extended_id", "standard_id"])

    if id_type == "extended_id":
        random_id = random.getrandbits(29)
    else:
        random_id = random.getrandbits(11)

    hex_id = hex(random_id).upper()[2:]

    remote_request = random.choice([True, False])

    num_bytes = random.randint(1, 8)
    random_hex = "".join(random.choices("0123456789abcdef", k=num_bytes * 2))
    data = ".".join(
        [random_hex[i : i + 2].upper() for i in range(0, len(random_hex), 2)]
    )

    frame = f"{hex_id}#R{num_bytes}" if remote_request else f"{hex_id}#{data}"

    return frame


def gen(device):
    """
    This function generates and sends random CAN frames to the specified device.

    Parameters:
    device (dict): A dictionary representing the CAN device. The dictionary should contain the following keys:
                   - "vendor" (str): The vendor of the CAN device. It can be either "anagate" or "socketcan".
                   - "host" (str): The host address for Anagate devices.
                   - "port" (int): The port number for Anagate devices.
                   - "device" (str): The device name for SocketCAN devices.

    Returns:
    None
    """
    configuration = process_device(device)
    arguments = CanDeviceArguments(configuration, None)
    can_device = CanDevice.create(device["vendor"], arguments)

    if (can_device.open() != CanReturnCode.success):
        print("Error opening CAN device")
        exit(1)
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


def diag(device):
    """
    Prints the diagnostics of the specified CAN device.

    This function takes a device dictionary as input, processes it to create a CanDeviceConfiguration object,
    and then creates a CanDevice object using the provided CAN module. The function opens the CAN device,
    retrieves the diagnostics, prints them, and finally closes the CAN device.

    Parameters:
    device (dict): A dictionary representing the CAN device. The dictionary should contain the following keys:
                   - "vendor" (str): The vendor of the CAN device. It can be either "anagate" or "socketcan".
                   - "host" (str): The host address for Anagate devices.
                   - "port" (int): The port number for Anagate devices.
                   - "device" (str): The device name for SocketCAN devices.

    Returns:
    None
    """
    configuration = process_device(device)
    arguments = CanDeviceArguments(configuration, None)
    can_device = CanDevice.create(device["vendor"], arguments)
    
    if (can_device.open() != CanReturnCode.success):
        print("Error opening CAN device")
        exit(1)
    
    print(can_device.diagnostics())
    can_device.close()


if __name__ == "__main__":
    main()

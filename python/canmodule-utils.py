import argparse
import time
from canmodule import *


def parse_arguments():
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
    configuration = process_device(device)
    arguments = CanDeviceArguments(configuration, print)
    can_device = CanDevice.create(device["vendor"], arguments)
    can_device.open()

    try:
        print("**** Dump started. Press CTRL+C to stop. ****")
        while True:
            time.sleep(5)
    except KeyboardInterrupt:
        print("\nCTRL+C pressed. Exiting gracefully...")
        can_device.close()


def send(device, frame):
    can_id = int(frame["can_id"], 16)

    extended_id = False
    if len(frame["can_id"]) > 3:
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
        print(f"Data: {data}")
        print(f"Can ID: {can_id} Data: {data} Flags: {flags}")
        can_frame = CanFrame(can_id, data, flags)

    configuration = process_device(device)
    arguments = CanDeviceArguments(configuration, None)
    can_device = CanDevice.create(device["vendor"], arguments)
    can_device.open()
    can_device.send(can_frame)
    print(f"Sent frame: {can_frame}")


def gen(device):
    print("Not implemented yet")


def diag(device):
    configuration = process_device(device)
    arguments = CanDeviceArguments(configuration, None)
    can_device = CanDevice.create(device["vendor"], arguments)
    can_device.open()
    print(can_device.diagnostics())
    can_device.close()


if __name__ == "__main__":
    main()

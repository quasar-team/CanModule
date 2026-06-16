import argparse
import sys

from .actions import diag, dump, gen, send
from .config import (
    build_can_device_configuration,
    merge_config_sources,
    validate_required_config,
)
from .frames import parse_can_frame


def extract_config_options(argv):
    remaining = []
    set_items = []
    config_file = None
    index = 0

    while index < len(argv):
        arg = argv[index]
        if arg == "--set":
            index += 1
            if index >= len(argv):
                raise ValueError("--set requires KEY=VALUE")
            set_items.append(argv[index])
        elif arg.startswith("--set="):
            set_items.append(arg.split("=", 1)[1])
        elif arg == "--config":
            index += 1
            if index >= len(argv):
                raise ValueError("--config requires FILE")
            config_file = argv[index]
        elif arg.startswith("--config="):
            config_file = arg.split("=", 1)[1]
        else:
            remaining.append(arg)
        index += 1

    return remaining, config_file, set_items


def build_parser():
    parser = argparse.ArgumentParser(description="CAN Module Utilities")
    subparsers = parser.add_subparsers(
        dest="vendor", required=True, help="sub-command help"
    )

    anagate_parser = subparsers.add_parser("anagate", help="Use the Anagate CAN module")
    add_action_parsers(anagate_parser, "Anagate")

    socketcan_parser = subparsers.add_parser(
        "socketcan", help="Use the SocketCAN module"
    )
    add_action_parsers(socketcan_parser, "SocketCAN")

    return parser


def add_action_parsers(vendor_parser, vendor_name):
    actions = vendor_parser.add_subparsers(
        dest="action", required=True, help=f"{vendor_name} actions"
    )

    actions.add_parser("dump", help="Dump frames")
    actions.add_parser("diag", help="Print diagnostics")
    actions.add_parser("gen", help="Generate and send random frames")

    send_parser = actions.add_parser("send", help="Send a specified frame")
    send_parser.add_argument("can_frame", type=str, help="CAN frame to send")


def parse_arguments(argv=None):
    if argv is None:
        argv = sys.argv[1:]

    parser = build_parser()

    try:
        argv, config_file, set_items = extract_config_options(list(argv))
    except ValueError as error:
        parser.error(str(error))

    parsed_args = parser.parse_args(argv)
    parsed_args.config_file = config_file
    parsed_args.set_items = set_items
    return parsed_args


def main(argv=None):
    args = parse_arguments(argv)

    try:
        merged_config = merge_config_sources(
            config_file=args.config_file,
            set_items=args.set_items,
        )
        validate_required_config(args.vendor, merged_config)
    except ValueError as error:
        raise SystemExit(str(error)) from error

    configuration = build_can_device_configuration(merged_config)

    if args.action == "dump":
        dump(args.vendor, configuration)
    elif args.action == "send":
        send(args.vendor, configuration, parse_can_frame(args.can_frame))
    elif args.action == "gen":
        gen(args.vendor, configuration)
    elif args.action == "diag":
        diag(args.vendor, configuration)

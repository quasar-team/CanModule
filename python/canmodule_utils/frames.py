import random

from canmodule import CanFrame, can_flags


def parse_can_frame(can_frame_str):
    if "#" not in can_frame_str:
        raise ValueError("Missing '#' delimiter")

    can_id, payload = can_frame_str.split("#", 1)
    if not can_id:
        raise ValueError("Missing CAN ID")

    try:
        can_id_value = int(can_id, 16)
    except ValueError as error:
        raise ValueError(f"Invalid CAN ID '{can_id}'") from error

    if can_id_value > 0x1FFFFFFF:
        raise ValueError(f"CAN ID 0x{can_id_value:X} is out of range (max 0x1FFFFFFF)")

    frame = {
        "can_id": can_id,
        # cansend-like rule: 8 hex digits force extended format even for small IDs.
        "id_format_hint": "extended" if len(can_id) == 8 else "auto",
    }

    if payload.startswith(("R", "r")):
        len_data = payload[1:]
        if len_data:
            try:
                requested_len = int(len_data)
            except ValueError as error:
                raise ValueError(
                    f"Invalid remote request length '{len_data}'"
                ) from error
        else:
            requested_len = 0

        if requested_len < 0 or requested_len > 8:
            raise ValueError("Remote request length must be between 0 and 8")

        frame["len"] = requested_len
    else:
        frame["data"] = payload.split(".")

    return frame


def process_frame(frame):
    can_id = int(frame["can_id"], 16)
    extended_id = frame.get("id_format_hint") == "extended" or can_id > 0x7FF
    flags = can_flags.extended_id if extended_id else can_flags.standard_id

    if "len" in frame.keys():
        flags |= can_flags.remote_request
        return CanFrame(can_id, frame["len"], flags)

    data = [chr(int(byte, 16)) for byte in frame["data"]]
    return CanFrame(can_id, data, flags)


def generate_random_frame():
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

    return f"{hex_id}#R{num_bytes}" if remote_request else f"{hex_id}#{data}"

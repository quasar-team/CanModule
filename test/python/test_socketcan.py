from time import sleep
import pytest
from common import *

pytestmark = pytest.mark.skipif(
    sys.platform.startswith("win"),
    reason="This test module is only for Linux environments.",
)

DEVICE_ONE = CanDeviceConfiguration()
DEVICE_ONE.bus_name = "vcan0"
DEVICE_TWO = CanDeviceConfiguration()
DEVICE_TWO.bus_name = "vcan0"


def test_socketcan_single_message():
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

    myDevice1.send(CanFrame(123, ["H", "e", "l", "l", "o"]))

    sleep(1)

    assert len(received_frames_dev2) == 1
    assert received_frames_dev2[0].id() == 123
    assert received_frames_dev2[0].message() == ["H", "e", "l", "l", "o"]

    assert len(received_frames_dev1) == 0


def test_socketcan_multiple_messages():
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

    send_frames = [
        CanFrame(123, ["H", "e", "l", "l", "o"]),
        CanFrame(234, ["W", "o", "r", "l", "d"]),
        CanFrame(345, 5),
        CanFrame(456, ["J", "u", "s", "t"], can_flags.extended_id),
        CanFrame(1 << 25, ["J", "u", "s", "t"], can_flags.extended_id),
    ]

    myDevice1.send(send_frames)

    sleep(1)

    assert len(received_frames_dev2) == 5

    assert received_frames_dev2[0].id() == 123
    assert received_frames_dev2[0].message() == ["H", "e", "l", "l", "o"]
    assert received_frames_dev2[0].is_extended_id() is False
    assert received_frames_dev2[0].is_remote_request() is False

    assert received_frames_dev2[1].id() == 234
    assert received_frames_dev2[1].message() == ["W", "o", "r", "l", "d"]
    assert received_frames_dev2[1].is_extended_id() is False
    assert received_frames_dev2[1].is_remote_request() is False

    assert received_frames_dev2[2].id() == 345
    assert received_frames_dev2[2].message() == []
    assert received_frames_dev2[2].length() == 5
    assert received_frames_dev2[2].is_extended_id() is False
    assert received_frames_dev2[2].is_remote_request() is True

    assert received_frames_dev2[3].id() == 456
    assert received_frames_dev2[3].message() == ["J", "u", "s", "t"]
    assert received_frames_dev2[3].is_extended_id() is True
    assert received_frames_dev2[3].is_remote_request() is False

    assert received_frames_dev2[4].id() == 1 << 25
    assert received_frames_dev2[4].message() == ["J", "u", "s", "t"]
    assert received_frames_dev2[4].is_extended_id() is True
    assert received_frames_dev2[4].is_remote_request() is False
    assert received_frames_dev2[4].is_error() is False
